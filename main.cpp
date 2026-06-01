#include <iostream>
#include <thread>
#include <tuple>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <array>

template<typename... Args>
struct get_size {};

template<typename T>
struct get_size<T> : std::integral_constant<std::size_t, sizeof(T)> {};

template<typename T, std::size_t Size>
struct get_size<std::array<T, Size>> : std::integral_constant<std::size_t, sizeof(T) * Size> {};

template<typename T, typename... Args>
struct get_size<T, Args...> : std::integral_constant<std::size_t, get_size<T>::value + get_size<Args...>::value> {};

template<typename... Args>
inline constexpr std::size_t get_size_v = get_size<Args...>::value;

template<typename T>
struct is_array : std::false_type {};

template<template<typename, std::size_t> class Tmp,
    typename T, std::size_t Size>
struct is_array<Tmp<T, Size>> : std::is_same<Tmp<T, Size>, std::array<T, Size>> {};

template<typename T>
struct get_array_type {};

template<template<typename, std::size_t> class Tmp, typename T, std::size_t Size>
struct get_array_type<Tmp<T, Size>> {
    using type = T;
};

template<typename T>
struct get_array_size : std::integral_constant<std::size_t, 0> {};

template<template<typename, std::size_t> class Tmp, typename T, std::size_t Size>
struct get_array_size<Tmp<T, Size>> : std::integral_constant<std::size_t, Size> {};

template<typename... Args>
struct payload_structure {
    static_assert((!std::is_pointer_v<Args> && ...));
    static_assert(((std::is_trivial_v<Args> || is_array<Args>::value) && ...));

    // 整个payload的大小
    static constexpr std::size_t size = get_size_v<Args...>;

    // 将参数格式化为二进制数据
    template<typename... FormatArgs, typename = std::enable_if_t<(std::is_same_v<std::decay_t<FormatArgs>, Args> && ...)>>
    static std::string format(FormatArgs&&... args) {
        std::string buffer;
        buffer.resize(size);
        std::size_t offset = 0;
        auto func = [&buffer, &offset](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;
            if constexpr (is_array<ArgType>::value) {
                write_array_to_buffer(buffer, offset, arg);
                offset += get_size_v<ArgType>;
            } else {
                write_to_buffer(buffer, offset, arg);
                offset += get_size_v<ArgType>;
            }
        };
        (..., func(std::forward<Args>(args)));
        return buffer;
    }

    // 从二进制数据中解析出参数
    static std::tuple<Args...> parse(const std::string& buffer) {
        if (buffer.size() != size) {
            throw std::runtime_error("Invalid buffer size");
        }
        std::size_t offset = 0;
        auto func = [&buffer, &offset](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;
            if constexpr (is_array<ArgType>::value) {
                auto array = read_array_from_buffer<typename get_array_type<ArgType>::type,
                    get_array_size<ArgType>::value>(buffer, offset);
                offset += get_size_v<ArgType>;
                return array;
            } else {
                auto value = read_from_buffer<ArgType>(buffer, offset);
                offset += get_size_v<ArgType>;
                return value;
            }
        };
        std::tuple<Args...> result;
        std::apply([&func](auto&&... args) {
            (..., (args = func(args)));
        }, result);
        return result;
    }

protected:
    template<typename T>
    static T convert_endian(T value) {
        if constexpr (std::endian::native == std::endian::big) {
            return value;
        } else {
            T result = 0;
            for (std::size_t i = 0; i < sizeof(T); ++i) {
                result |= ((value >> (i * 8)) & 0xFF) << ((sizeof(T) - 1 - i) * 8);
            }
            return result;
        }
    }

    template<typename T>
    static T read_from_buffer(const std::string& buffer, std::size_t offset) {
        T value;
        std::memcpy(&value, buffer.data() + offset, sizeof(T));
        return convert_endian(value);
    }

    template<typename T>
    static void write_to_buffer(std::string& buffer, std::size_t offset, const T& value) {
        T converted = convert_endian(value);
        std::memcpy(buffer.data() + offset, &converted, sizeof(T));
    }

    template<typename T, std::size_t Size>
    static std::array<T, Size> read_array_from_buffer(const std::string& buffer, std::size_t offset) {
        std::array<T, Size> array;
        for (std::size_t i = 0; i < Size; ++i) {
            array[i] = read_from_buffer<T>(buffer, offset + i * sizeof(T));
        }
        return array;
    }

    template<typename T, std::size_t Size>
    static void write_array_to_buffer(std::string& buffer, std::size_t offset, const std::array<T, Size>& array) {
        for (std::size_t i = 0; i < Size; ++i) {
            write_to_buffer(buffer, offset + i * sizeof(T), array[i]);
        }
    }
};

int main() {
    auto size = get_size_v<decltype(int16_t(1)), decltype(int16_t(2)), decltype(int16_t(3))>;
    is_array<std::array<char, 10>>::value;
    using Payload = payload_structure<int16_t, std::array<char, 12>>;
    std::array<char, 12> data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '\0'};
    auto str = Payload::format(int16_t(1), data);
    auto tuple = Payload::parse(str);
    std::cout << str << std::endl;
    std::cout << std::get<0>(tuple) << std::endl;
    std::cout << std::get<1>(tuple).data() << std::endl;
    return 0;
}

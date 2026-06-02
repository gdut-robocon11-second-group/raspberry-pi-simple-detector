#include "payload_structure.hpp"
#include <iostream>

int main() {
  auto size = get_size_v<decltype(int16_t(1)), decltype(int16_t(2)),
                         decltype(int16_t(3))>;
  using Payload = payload_structure<int16_t, std::array<char, 12>>;
  std::string data = "Hello World!!";
  Payload::format(format_to_array, 1ll, data);
  auto str = Payload::format(format_to_string, 1ll, data);
  auto tuple = Payload::parse(str);
  std::cout << str << std::endl;
  std::cout << std::get<0>(tuple) << std::endl;
  std::cout << std::get<1>(tuple).data() << std::endl;
  return 0;
}

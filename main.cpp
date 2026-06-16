#include "detect_functions.hpp"
#include "payload_structure.hpp"
#include "serial_connector.hpp"
#include "sound_player.hpp"
#include <asio.hpp>
#include <iostream>


class detector_node {
public:
  using uint8_payload = payload_structure<std::uint8_t>;

  detector_node(asio::io_context &io_context, cv::VideoCapture &cap,
                const std::string &portName)
      : io_context_(io_context), detector_(cap),
        serial_connector_(portName, io_context) {}
  ~detector_node() = default;

  enum class command_code : std::uint16_t {
    request_traffic_light_status = 0x10,
    request_transportation = 0x11,
    send_departure_station = 0x12,

    send_open_door = 0x00,
    send_traffic_light_status = 0x01,
    send_transportation = 0x02,
    send_open_door_at_departure = 0x03
  };

  enum class goods_code : std::uint8_t { goods1 = 0x01, goods2 = 0x02 };

  enum class traffic_light_status : std::uint8_t {
    red = 0x00,
    yellow = 0x01,
    green = 0x02
  };

  enum class transportation : std::uint8_t { left = 0x00, right = 0x01 };

  template <command_code Command, typename Handler, typename... Args>
  auto send_command(Handler &&handler, Args &&...args) {
    if constexpr (Command == command_code::send_open_door) {
      static_assert(sizeof...(Args) == 1,
                    "send_open_door requires one argument");
      auto packet = uint8_payload::format(
          format_to_array,
          static_cast<std::uint8_t>(std::forward<Args>(args))...);
      serial_connector::packet_t packet_data(
          static_cast<std::uint16_t>(Command), packet.begin(), packet.end(),
          gdut::build_packet);
      return serial_connector_.async_send(packet_data,
                                          std::forward<Handler>(handler));

    } else if constexpr (Command == command_code::send_traffic_light_status) {
      static_assert(sizeof...(Args) == 1,
                    "send_traffic_light_status requires one argument");
      auto packet = uint8_payload::format(
          format_to_array,
          static_cast<std::uint8_t>(std::forward<Args>(args))...);
      serial_connector::packet_t packet_data(
          static_cast<std::uint16_t>(Command), packet.begin(), packet.end(),
          gdut::build_packet);
      return serial_connector_.async_send(packet_data,
                                          std::forward<Handler>(handler));

    } else if constexpr (Command == command_code::send_transportation) {
      static_assert(sizeof...(Args) == 1,
                    "send_transportation requires one argument");
      auto packet = uint8_payload::format(
          format_to_array,
          static_cast<std::uint8_t>(std::forward<Args>(args))...);
      serial_connector::packet_t packet_data(
          static_cast<std::uint16_t>(Command), packet.begin(), packet.end(),
          gdut::build_packet);
      return serial_connector_.async_send(packet_data,
                                          std::forward<Handler>(handler));

    } else if constexpr (Command == command_code::send_open_door_at_departure) {
      static_assert(sizeof...(Args) == 0,
                    "send_open_door_at_departure requires no arguments");
      serial_connector::packet_t packet_data(
          static_cast<std::uint16_t>(Command), nullptr, nullptr,
          gdut::build_packet);
      return serial_connector_.async_send(packet_data,
                                          std::forward<Handler>(handler));

    } else {
      static_assert(always_false<decltype(Command)>::value,
                    "Unsupported command");
    }
  }

  asio::awaitable<void> receive_loop() {
    while (true) {
      auto packet =
          co_await serial_connector_.async_receive(asio::use_awaitable);
      switch (static_cast<command_code>(packet.code())) {
      case command_code::request_traffic_light_status: {
        auto traffic_light_status = detector_.detectTrafficLight();
        switch (traffic_light_status) {
        case Detector::TrafficLights::RED:
          std::cout << "Detected traffic light status: red" << '\n';
          co_await send_command<command_code::send_traffic_light_status>(
              asio::use_awaitable,
              static_cast<std::uint8_t>(traffic_light_status::red));
          break;
        case Detector::TrafficLights::YELLOW:
          std::cout << "Detected traffic light status: yellow" << '\n';
          co_await send_command<command_code::send_traffic_light_status>(
              asio::use_awaitable,
              static_cast<std::uint8_t>(traffic_light_status::yellow));
          break;
        case Detector::TrafficLights::GREEN:
          std::cout << "Detected traffic light status: green" << '\n';
          co_await send_command<command_code::send_traffic_light_status>(
              asio::use_awaitable,
              static_cast<std::uint8_t>(traffic_light_status::green));
          break;
        default:
          std::cout << "Detected traffic light status: unknown" << '\n';
          break;
        }
        break;
      }
      case command_code::request_transportation: {
        co_await send_command<command_code::send_transportation>(
            asio::use_awaitable,
            static_cast<std::uint8_t>(
                current_transportation_.load(std::memory_order_acquire)));
        break;
      }
      case command_code::send_departure_station: {
        std::cout << "Received departure station information" << '\n';
        auto tr = get_current_transportation();
        voice_player::get_instance().play_voice(
            (tr == transportation::left) ? voice_player::Voice::Voice3
                                         : voice_player::Voice::Voice4);
        asio::post(io_context_, [&]() {
          for (;;) {
            if (auto opt = detector_.detectQRCode(); opt.has_value()) {
              auto goods = get_current_goods();
              auto voice = (goods == goods_code::goods1)
                               ? voice_player::Voice::Voice5
                               : voice_player::Voice::Voice6;
              voice_player::get_instance().play_voice(voice);
              return;
            }
          }
        });
        break;
      }
      default:
        std::cerr << "Unknown command code received: " << packet.code() << '\n';
      }
    }
  }

  Detector &detector() { return detector_; }

  const Detector &detector() const { return detector_; }

  void set_current_transportation(transportation t) {
    current_transportation_.store(t, std::memory_order_release);
  }

  transportation get_current_transportation() const {
    return current_transportation_.load(std::memory_order_acquire);
  }

  void set_current_goods(goods_code g) {
    current_goods_.store(g, std::memory_order_release);
  }

  goods_code get_current_goods() const {
    return current_goods_.load(std::memory_order_acquire);
  }

protected:
  template <typename Arg> struct always_false : std::false_type {};

private:
  asio::io_context &io_context_;
  Detector detector_;
  serial_connector serial_connector_;
  std::atomic<transportation> current_transportation_{transportation::left};
  std::atomic<goods_code> current_goods_{goods_code::goods1};
};

inline static constexpr int capture_device_index = 0;

int main() {
  asio::io_context io_context;
  cv::VideoCapture cap(capture_device_index);
  detector_node node(io_context, cap, "/dev/ttyUSB0");
  asio::post(io_context, [&]() {
    while (!cap.isOpened()) {
      std::cerr
          << "Failed to open video capture device. Retrying in 1 second..."
          << '\n';
      std::this_thread::sleep_for(std::chrono::seconds(1));
      cap.open(capture_device_index);
    }
    for (;;) {
      if (auto opt = node.detector().detectQRCode(); opt.has_value()) {
        if (opt.value().size() != 2) {
          continue; // Invalid QR code format, ignore
        }
        std::cout << "Detected QR code: " << opt.value() << '\n';
        auto goods = (opt.value()[0] == '1')
                         ? detector_node::goods_code::goods1
                         : detector_node::goods_code::goods2;
        auto transportation = (opt.value()[1] == '1')
                                  ? detector_node::transportation::left
                                  : detector_node::transportation::right;
        auto voice = (opt.value()[0] == '1') ? voice_player::Voice::Voice1
                                             : voice_player::Voice::Voice2;
        node.set_current_goods(goods);
        node.set_current_transportation(transportation);
        node.send_command<detector_node::command_code::send_open_door>(
            asio::detached, static_cast<std::uint8_t>(goods));
        voice_player::get_instance().play_voice(voice);
        return;
      }
    }
  });
  asio::co_spawn(io_context, node.receive_loop(), asio::detached);
  io_context.run();
  return 0;
}

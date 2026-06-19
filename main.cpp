#include "detect_functions.hpp"
#include "detect_node.hpp"

#include <asio.hpp>
#include <opencv2/opencv.hpp>

inline static int capture_device_index = 0;
inline static std::string serial_port_name = "/dev/ttyAMA0";
inline static std::string audio_device_name = "plughw:1,0";

int main(int argc, char *argv[]) {
  if (argc > 3) {
    try {
      capture_device_index = std::stoi(argv[1]);
      serial_port_name = argv[2];
      audio_device_name = argv[3];
      sound_player::get_instance().set_audio_device(audio_device_name);
    } catch (const std::exception &e) {
      std::cerr << "Invalid arguments. Usage: " << argv[0]
                << " <capture_device_index> <serial_port_name> <audio_device_name>\n";
      return 1;
    }
  } else {
    std::cerr << "Usage: " << argv[0] << " <capture_device_index> <serial_port_name> <audio_device_name>\n";
    return 1;
  }
  try {
    asio::io_context io_context;
    cv::VideoCapture cap(capture_device_index, cv::CAP_V4L2);
    detector_node node(io_context, cap, serial_port_name);
    asio::post(io_context, [&]() {
      while (!cap.isOpened()) {
        std::cerr
            << "Failed to open video capture device. Retrying in 1 second..."
            << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cap.open(capture_device_index, cv::CAP_V4L2);
      }
      cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
      cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
      cap.set(cv::CAP_PROP_FPS, 30);
      for (;;) {
        if (auto opt = node.detector().detectQRCode(); opt.has_value()) {
          if (opt.value().size() != 2) {
            continue; // Invalid QR code format, ignore
          }
          if (opt.value()[0] != '1' && opt.value()[0] != '2') {
            continue; // Invalid goods code, ignore
          }
          if (opt.value()[1] != '1' && opt.value()[1] != '2') {
            continue; // Invalid transportation code, ignore
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
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << '\n';
    return 1;
  }
  return 0;
}

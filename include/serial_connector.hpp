#pragma once

#include "transfer_protocol.hpp"
#include <array>
#include <asio.hpp>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


class serial_connector {
public:
  using packet_t = gdut::packet_manager<gdut::crc16_algorithm>::packet_t;

  serial_connector(const std::string &port_name, asio::io_context &io_context)
      : serial_port_(io_context, port_name),
        strand_(io_context.get_executor()) {
    // 启动串口连接
    serial_port_.set_option(asio::serial_port_base::baud_rate(115200));
    serial_port_.set_option(asio::serial_port_base::character_size(8));
    serial_port_.set_option(
        asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial_port_.set_option(asio::serial_port_base::stop_bits(
        asio::serial_port_base::stop_bits::one));
    serial_port_.set_option(asio::serial_port_base::flow_control(
        asio::serial_port_base::flow_control::none));

    if (!serial_port_.is_open()) {
      throw std::runtime_error("Failed to open serial port");
    }
  }

  // packet需要求调用者保证其生命周期至少持续到异步操作完成
  template <asio::completion_token_for<void(std::error_code, std::size_t)>
                WriteHandler>
  auto async_send(const packet_t &packet, WriteHandler &&handler) {
    return asio::async_write(
        serial_port_,
        asio::buffer(packet.begin(),
                     std::distance(packet.begin(), packet.end())),
        asio::bind_executor(strand_, std::forward<WriteHandler>(handler)));
  }

  template <
      asio::completion_token_for<void(std::error_code, packet_t)> ReadHandler>
  auto async_receive(ReadHandler &&handler) {
    return asio::async_initiate<ReadHandler, void(std::error_code, packet_t)>(
        [this](auto &&completion_handler) {
          start_receive(
              std::forward<decltype(completion_handler)>(completion_handler));
        },
        std::forward<ReadHandler>(handler));
  }

  ~serial_connector() noexcept {
    serial_port_.cancel();
    serial_port_.close();
  }

  operator bool() const noexcept { return serial_port_.is_open(); }

protected:
  template <typename Handler>
  void
  start_receive(Handler &&handler,
                std::shared_ptr<std::array<uint8_t, 1024>> buffer = nullptr) {
    if (std::unique_lock<std::mutex> lock{handler_mutex_};
        packet_manager_.has_packet()) {
      auto packet = packet_manager_.pop_packet();
      lock.unlock();
      asio::dispatch(strand_, [handler = std::forward<Handler>(handler),
                               packet = std::move(packet)]() mutable {
        handler(std::error_code{}, std::move(packet));
      });
      return;
    }

    if (!buffer) {
      buffer = std::make_shared<std::array<uint8_t, 1024>>();
    }
    serial_port_.async_read_some(
        asio::buffer(*buffer),
        asio::bind_executor(
            strand_,
            [this, buffer, handler = std::forward<Handler>(handler)](
                std::error_code ec, std::size_t bytes_transferred) mutable {
              if (ec) {
                handler(ec, packet_t{});
                return;
              }

              std::unique_lock<std::mutex> lock{handler_mutex_};
              packet_manager_.receive(buffer->begin(),
                                      buffer->begin() + bytes_transferred);
              if (packet_manager_.has_packet()) {
                auto packet = packet_manager_.pop_packet();
                lock.unlock();
                handler(std::error_code{}, std::move(packet));
              } else {
                lock.unlock();
                start_receive(std::move(handler), std::move(buffer));
              }
            }));
  }

private:
  mutable std::mutex handler_mutex_;
  gdut::packet_manager<gdut::crc16_algorithm> packet_manager_;
  asio::serial_port serial_port_;
  asio::strand<asio::any_io_executor> strand_;
};

#pragma once

#include <asio.hpp>

#include "serial_connector.hpp"

class DetectNode {
public:
    enum class Code {
        Unknown = 0
    };

    DetectNode(asio::io_context& io_context, const std::string &port_name) : m_serial_connector(port_name, io_context) {

    }

    ~DetectNode() = default;

protected:
    template<typename... Args>
    std::string format(Args&&... args) {
        std::string buffer;
        auto get_size = []<typename Arg>(std::size_t size) {
            return size+sizeof(Arg);
        }
        std::size_t size = (..., get_size<Args>(0));
        auto func = [](auto&& arg) {

        }
    }

private:
    SerialConnector m_serial_connector;
};

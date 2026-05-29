cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(raspberry-pi-simple-detector)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

find_package(OpenCV REQUIRED)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/asio/include)

add_executable(raspberry-pi-simple-detector
    main.cpp
    detect_node.hpp
    detect_functions.hpp
    sound_player.hpp
    transfer_protocol.hpp
    verification_algorithm.hpp)
target_include_directories(raspberry-pi-simple-detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(raspberry-pi-simple-detector PRIVATE
    asio
    ${OpenCV_LIBS})

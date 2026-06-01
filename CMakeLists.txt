cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(raspberry-pi-simple-detector)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

find_package(OpenCV REQUIRED)

add_subdirectory(miniaudio)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/asio/include)

file(GLOB INCLUDE_FILES "include/*.hpp")

add_executable(raspberry-pi-simple-detector
    main.cpp
    ${INCLUDE_FILES}
)
target_include_directories(raspberry-pi-simple-detector PRIVATE
    include
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(raspberry-pi-simple-detector PRIVATE
    asio
    miniaudio
    ${OpenCV_LIBS}
)

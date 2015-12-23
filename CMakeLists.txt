cmake_minimum_required(VERSION 2.8)

project(echo_test)

#if (MSVC)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
#endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

include_directories(.)

add_subdirectory(gtest)

add_library(http STATIC
    http_parser.cpp
    http_printer.cpp
    sub_string.cpp)

add_library(common STATIC
    address.cpp
    epoll.cpp
    pipe.cpp
    socket.cpp
    throw_error.cpp
    timer.cpp
    file_descriptor.cpp
    http_server.cpp
)

add_executable(echo_server
    main_echo_server.cpp
    echo_server.cpp
)

target_link_libraries(echo_server common)

add_executable(echo_test
    main_echo_test.cpp
    echo_tester.cpp
)

target_link_libraries(echo_test common)

add_executable(http_parser_test
    http_parser_test.cpp
)

target_link_libraries(http_parser_test http gtest)
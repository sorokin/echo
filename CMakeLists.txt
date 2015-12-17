cmake_minimum_required(VERSION 2.8)

project(echo_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

IF(UNIX)
    IF(APPLE)
    add_library(common STATIC
        address.cpp
        kqueue.cpp
        socket_apple.cpp
        throw_error.cpp
        timer.cpp
        file_descriptor.cpp   
        )
    ELSE(APPLE)
    add_library(common STATIC
        address.cpp
        epoll.cpp
        socket.cpp
        throw_error.cpp
        timer.cpp
        file_descriptor.cpp
        )
    ENDIF(APPLE)
ENDIF(UNIX)

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

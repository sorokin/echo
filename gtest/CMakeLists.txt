cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

#include_directories(..)

add_library(gtest STATIC
    gtest.h
    gtest_main.cc
    gtest-all.cc)

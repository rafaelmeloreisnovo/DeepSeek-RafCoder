cmake_minimum_required(VERSION 3.22.1)
project(rafcoder_native)

add_library(rafcoder_native SHARED native-lib.cpp)

find_library(log-lib log)

target_link_libraries(rafcoder_native ${log-lib})

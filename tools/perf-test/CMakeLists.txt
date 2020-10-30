cmake_minimum_required(VERSION 3.0)
project(perf-test)

include_directories(.)
add_executable(perf-test ./main.c)
target_link_libraries(perf-test PRIVATE asound)

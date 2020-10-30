cmake_minimum_required(VERSION 3.0)
project(midibridge-tools)

include_directories(.)
add_executable(perf-test main.c)
target_link_libraries(perf-test PRIVATE asound)

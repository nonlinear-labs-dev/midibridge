cmake_minimum_required(VERSION 3.0)
project(midibridge-tools)

include_directories(perf-test)
add_executable(perf-test perf-test/main.c)
target_link_libraries(perf-test PRIVATE asound)

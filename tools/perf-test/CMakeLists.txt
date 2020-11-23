cmake_minimum_required(VERSION 3.0)
project(midibridge-tools)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror")

include_directories(.)
add_executable(perf-test main.c)
target_link_libraries(perf-test PRIVATE asound)

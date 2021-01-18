cmake_minimum_required(VERSION 3.0)
project(midibridge-tools)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror")

include_directories(.)
add_executable(perf-test perf-test.c)
target_link_libraries(perf-test PRIVATE asound)

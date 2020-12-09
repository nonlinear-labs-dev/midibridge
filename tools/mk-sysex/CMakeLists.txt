cmake_minimum_required(VERSION 3.0)
project(mk-sysex)

include_directories(src ../../firmware/src/shared)
add_executable(mk-sysex src/main.c)

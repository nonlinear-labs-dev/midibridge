# input variables: FIRMWARE_DIRNAME

cmake_minimum_required(VERSION 3.2)

project(mk-sysex)

include_directories(src ../../${FIRMWARE_DIRNAME}/src/shared)
add_executable(mk-sysex src/mk-sysex.c)

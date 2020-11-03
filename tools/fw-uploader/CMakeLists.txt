cmake_minimum_required(VERSION 3.0)
project(nlmb-fwupload)

include_directories(src ../../firmware/src/shared)
add_executable(nlmb-fwupload src/main.c ../../firmware/src/shared/midi/nl_sysex.c)
target_link_libraries(nlmb-fwupload PRIVATE asound)

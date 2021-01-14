set(BLOBNAME "sysex-isp")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-builtin -nodefaultlibs -nostdlib")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -fmessage-length=0 -ffunction-sections -fdata-sections ")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-print-memory-usage")

add_compile_options("-DCORE_M4")

file(GLOB_RECURSE SOURCES ./src/*.c ../shared/*.c)

add_executable(${BLOBNAME} ${SOURCES} ../main/main.o)

set_source_files_properties(../main/main.o PROPERTIES GENERATED TRUE)

target_include_directories (${BLOBNAME} PRIVATE "./src")

target_link_options(${BLOBNAME} PUBLIC -Wl,-Map,output.map -Wl,--gc-sections)
target_link_directories(${BLOBNAME} PUBLIC ${CMAKE_SOURCE_DIR}/firmware/ld)

set_target_properties(${BLOBNAME} PROPERTIES LINKER_LANGUAGE C)
set_target_properties(${BLOBNAME} PROPERTIES LINK_FLAGS "-Tsysex-isp.ld")
set_target_properties(${BLOBNAME} PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")

add_custom_command(OUTPUT ${BLOBNAME}.syx
    DEPENDS ${BLOBNAME}
    DEPENDS main.image
    DEPENDS mk-sysex
    COMMAND arm-none-eabi-objcopy --verbose --strip-all -O binary --remove-section=.ARM.attributes --remove-section=".bss*" --remove-section=".noinit*" ${BLOBNAME} ${BLOBNAME}.bin
    COMMAND mk-sysex ${BLOBNAME}.bin ${BLOBNAME}.syx
)

add_custom_target(${BLOBNAME}.image ALL
    COMMENT "Create ${BLOBNAME}.syx"
    DEPENDS ${BLOBNAME}.syx
)

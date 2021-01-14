set(BLOBNAME "main")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-builtin -nodefaultlibs -nostdlib")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -fmessage-length=0 -ffunction-sections -fdata-sections ")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-print-memory-usage")

add_compile_options("-DCORE_M4")

file(GLOB_RECURSE SOURCES ./src/*.c ../shared/*.c)

add_executable(${BLOBNAME} ${SOURCES})

target_include_directories (${BLOBNAME} PRIVATE "./src")

target_link_options(${BLOBNAME} PUBLIC -Wl,-Map,output.map -Wl,--gc-sections)
target_link_directories(${BLOBNAME} PUBLIC ${CMAKE_SOURCE_DIR}/firmware/ld)

set_target_properties(${BLOBNAME} PROPERTIES LINKER_LANGUAGE C)
set_target_properties(${BLOBNAME} PROPERTIES LINK_FLAGS "-Tmain.ld")
set_target_properties(${BLOBNAME} PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")

add_custom_command(OUTPUT ${BLOBNAME}.o
    DEPENDS ${BLOBNAME}
    COMMAND arm-none-eabi-objcopy --verbose --strip-all -O binary --remove-section=.ARM.attributes --remove-section=".bss*" --remove-section=".noinit*" ${BLOBNAME} ${BLOBNAME}.bin
    COMMAND arm-none-eabi-objcopy -v -I binary -O elf32-littlearm -B arm ${BLOBNAME}.bin ${BLOBNAME}.o
)

add_custom_target(${BLOBNAME}.image ALL
    COMMENT "Create ${BLOBNAME}.o"
    DEPENDS ${BLOBNAME}.o
)

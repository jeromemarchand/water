cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(water C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# Initialize the SDK
pico_sdk_init()

add_executable(water
  water.c
)

pico_enable_stdio_usb(water 1)
pico_enable_stdio_uart(water 1)

pico_add_extra_outputs(water)

target_link_libraries(water pico_stdlib hardware_adc)

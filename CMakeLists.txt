cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(lorago C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(lorago
  lorago.c
)
pico_enable_stdio_usb(lorago 1)
pico_enable_stdio_uart(lorago 0)
pico_add_extra_outputs(lorago)
target_link_libraries(lorago pico_stdlib hardware_spi)


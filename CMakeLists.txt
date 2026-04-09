cmake_minimum_required(VERSION 3.13)

# Pico SDK import
include(pico_sdk_import.cmake)

project(tipsy_rp2350 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize Pico SDK
pico_sdk_init()

# Main executable
add_executable(tipsy_rp2350
    src/main.c
)

# Link libraries
target_link_libraries(tipsy_rp2350
    pico_stdlib
)

# Enable USB output, disable UART
pico_enable_stdio_usb(tipsy_rp2350 1)
pico_enable_stdio_uart(tipsy_rp2350 0)

# Create UF2 output (minimal - avoids picotool dependency)
pico_add_uf2_output(tipsy_rp2350)

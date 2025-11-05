cmake_minimum_required(VERSION 3.13)

# Set board to Pico2 (RP2350) - MUST be before project()
set(PICO_BOARD pico2)

# Pull in SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(matrix_keypad C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add executable
add_executable(matrix_keypad
    main_robust_example.c
    matrix_robust.c
    keymap_functions.c
)

# Pull in common dependencies
target_link_libraries(matrix_keypad 
    pico_stdlib
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(matrix_keypad 1)
pico_enable_stdio_uart(matrix_keypad 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(matrix_keypad)


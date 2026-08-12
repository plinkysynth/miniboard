# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(sw2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

option(SW2_ENABLE_AFTERTOUCH "Enable Fatar channel aftertouch on ADC0 / GPIO26" ON)
option(SW2_MIDI_TYPE_B "Use TRS MIDI Type B instead of Type A" OFF)

# Add executable. Default name is the project name, version 0.1

add_executable(sw2 sw2.c)
pico_generate_pio_header(sw2 ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)

pico_set_program_name(sw2 "sw2")
pico_set_program_version(sw2 "0.1")

target_compile_definitions(sw2 PRIVATE
        ENABLE_AFTERTOUCH=$<BOOL:${SW2_ENABLE_AFTERTOUCH}>
        MIDI_TYPE_B=$<BOOL:${SW2_MIDI_TYPE_B}>)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(sw2 0)
pico_enable_stdio_usb(sw2 0)
pico_enable_stdio_rtt(sw2 1)

# Add the standard library to the build
target_link_libraries(sw2
        pico_stdlib
        hardware_adc
        hardware_pio
        tinyusb_device)

# Add the standard include files to the build
target_include_directories(sw2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(sw2)

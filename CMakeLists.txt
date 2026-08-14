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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(DVI_video_test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(software/libdvi libdvi)
add_subdirectory(software/libsprite libsprite)

# Add executable. Default name is the project name, version 0.1

add_executable(DVI_video_test DVI_video_test.c )

pico_set_program_name(DVI_video_test "DVI_video_test")
pico_set_program_version(DVI_video_test "0.1")

# Generate PIO header
pico_generate_pio_header(DVI_video_test ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(DVI_video_test 1)
pico_enable_stdio_usb(DVI_video_test 1)

# Add the standard library to the build
target_link_libraries(DVI_video_test
        pico_stdlib
        hardware_pio
        hardware_dma    # Requis pour le transfert vidéo fluide
        hardware_vreg   # Requis pour monter la tension (overclock 252MHz)
        hardware_clocks
        pico_multicore
        libdvi          # La bibliothèque PicoDVI elle-même
        libsprite       # La bibliothèque sprite
        )

# Add the standard include files to the build
target_include_directories(DVI_video_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(DVI_video_test)

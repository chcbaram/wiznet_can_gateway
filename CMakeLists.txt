cmake_minimum_required(VERSION 3.12)

set(PRJ_NAME ethernet_can_gateway)

set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/external/pico-sdk")
set(WIZNET_DIR "${CMAKE_SOURCE_DIR}/external/ioLibrary_Driver")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${PRJ_NAME})

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized 
        )

add_subdirectory(src)
add_subdirectory(external)

# CMake minimum required version
cmake_minimum_required(VERSION 3.12)


include(rp2040_hat_c-patch.cmake)
include(rp2040_hat_c_sdk_version.cmake)

# Set the project root directory if it's not already defined, as may happen if
# the tests folder is included directly by a parent project, without including
# the top level CMakeLists.txt.
if(NOT DEFINED WIZNET_DIR)
    set(WIZNET_DIR ${CMAKE_SOURCE_DIR}/external/ioLibrary_Driver)
    message(STATUS "WIZNET_DIR = ${WIZNET_DIR}")
endif()

if(NOT DEFINED PORT_DIR)
    set(PORT_DIR ${CMAKE_SOURCE_DIR}/src/lib/RP2040-HAT-C/port)
    message(STATUS "PORT_DIR = ${PORT_DIR}")
endif()


# Add libraries in subdirectories
add_subdirectory(${PORT_DIR})



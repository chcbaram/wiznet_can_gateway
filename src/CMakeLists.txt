
file(GLOB SRC_FILES CONFIGURE_DEPENDS
  *.cpp
)

file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  ap/*.cpp
  bsp/*.c
  common/*.c
  hw/*.c
)


add_executable(
  ${PRJ_NAME}

  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
)

target_include_directories(${PRJ_NAME} PUBLIC 
  ap
  ap/include
  bsp
  common 
  common/core
  common/hw/include 
  hw
)

target_link_libraries(${PRJ_NAME} 
  pico_stdlib
  hardware_spi
  hardware_dma
  hardware_i2c
  pico_multicore

  ETHERNET_FILES
  SPI_FILES
  LOOPBACK_FILES 
  HTTPSERVER_FILES
  DHCP_FILES
  DNS_FILES
  TIMER_FILES
  )

pico_add_extra_outputs(${PRJ_NAME})
pico_enable_stdio_usb(${PRJ_NAME}  1)
pico_enable_stdio_uart(${PRJ_NAME} 0)

add_subdirectory(lib/RP2040-HAT-C)

set_target_properties(${PRJ_NAME} PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET ${PRJ_NAME} POST_BUILD
        COMMAND arm-none-eabi-size --format=berkeley --totals "$<TARGET_FILE:${PRJ_NAME}>"
        COMMENT "Invoking: Cross ARM GNU Print Size")
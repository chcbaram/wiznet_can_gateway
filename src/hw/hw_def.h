#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V211107R1"
#define _DEF_BOARD_NAME           "RPI_PICO"





#define _USE_HW_LED
#define      HW_LED_MAX_CH          1

#define _USE_HW_BUTTON
#define      HW_BUTTON_MAX_CH       1
#define      HW_BUTTON_OBJ_USE      1

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1

#define _USE_HW_GPIO
#define      HW_GPIO_MAX_CH         2

#define _USE_HW_SPI
#define      HW_SPI_MAX_CH          1

#define _USE_HW_I2C
#define      HW_I2C_MAX_CH          1

#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    16
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    4
#define      HW_CLI_LINE_BUF_MAX    64

#define _USE_HW_LOG
#define      HW_LOG_CH              _DEF_UART1
#define      HW_LOG_BOOT_BUF_MAX    1024
#define      HW_LOG_LIST_BUF_MAX    1024

#define _USE_HW_MCP2515
#define      HW_MCP2515_MAX_CH      1

#define _USE_HW_CAN
#define      HW_CAN_MAX_CH          1
#define      HW_CAN_MSG_RX_BUF_MAX  16

#define _USE_HW_LCD
#define _USE_HW_SSD1306
#define      HW_LCD_WIDTH           128
#define      HW_LCD_HEIGHT          32

#define _USE_HW_CMD_CAN
#define      HW_CMD_CAN_MAX_DATA_LENGTH 1024


#endif /* SRC_HW_HW_DEF_H_ */
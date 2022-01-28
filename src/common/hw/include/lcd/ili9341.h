/*
 * ili9341.h
 *
 *  Created on: 2020. 3. 26.
 *      Author: HanCheol Cho
 */

#ifndef SRC_COMMON_HW_INCLUDE_ILI9341_H_
#define SRC_COMMON_HW_INCLUDE_ILI9341_H_



#ifdef __cplusplus
 extern "C" {
#endif



#include "hw_def.h"


#ifdef _USE_HW_ILI9341

#include "lcd.h"
#include "ili9341_regs.h"



#define ILI9341_LCD_WIDTH      LCD_WIDTH
#define ILI9341_LCD_HEIGHT     LCD_HEIGHT


bool ili9341Init(void);
bool ili9341InitDriver(lcd_driver_t *p_driver);


#endif

#ifdef __cplusplus
}
#endif



#endif /* SRC_COMMON_HW_INCLUDE_ILI9341_H_ */

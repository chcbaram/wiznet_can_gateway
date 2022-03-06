/*
 * hw.c
 *
 *  Created on: Jun 13, 2021
 *      Author: baram
 */

#include "hw.h"





bool hwInit(void)
{  
  bspInit();

  cliInit();
  logInit();
  ledInit();
  gpioInit();
  buttonInit();
  spiInit();
  i2cInit();
  uartInit();
  uartOpen(_DEF_UART1, 57600);
  logOpen(_DEF_UART1, 57600);

  logPrintf("[ Firmware Begin... ]\r\n");
  logPrintf("Booting..Name \t\t: %s\r\n", _DEF_BOARD_NAME);
  logPrintf("Booting..Ver  \t\t: %s\r\n", _DEF_FIRMWATRE_VERSION);
  logPrintf("Clk sys  \t\t: %d\r\n", clock_get_hz(clk_sys));
  logPrintf("Clk peri \t\t: %d\r\n", clock_get_hz(clk_peri));
  logPrintf("Clk usb  \t\t: %d\r\n", clock_get_hz(clk_usb));
  logPrintf("Clk adc  \t\t: %d\r\n", clock_get_hz(clk_adc));
  logPrintf("Clk rtc  \t\t: %d\r\n", clock_get_hz(clk_rtc));
  logPrintf("Clk timer\t\t: %d\r\n", clock_get_hz(clk_ref));
  
  canInit();
  lcdInit();
  lcdSetFps(20);

  logBoot(false);

  return true;
}
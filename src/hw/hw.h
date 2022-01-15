/*
 * hw.h
 *
 *  Created on: Jun 13, 2021
 *      Author: baram
 */

#ifndef SRC_HW_HW_H_
#define SRC_HW_HW_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"



#include "led.h"
#include "uart.h"
#include "cli.h"
#include "log.h"
#include "gpio.h"
#include "spi.h"
#include "can.h"


bool hwInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_HW_HW_H_ */
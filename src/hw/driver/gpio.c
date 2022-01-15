/*
 * gpio.c
 *
 *  Created on: 2020. 12. 24.
 *      Author: baram
 */


#include "gpio.h"
#include "cli.h"


#ifdef _USE_HW_GPIO

typedef struct
{
  uint32_t      pin;
  uint8_t       mode;
  uint8_t       on_state;
  uint8_t       off_state;
  bool          init_value;
} gpio_tbl_t;


const gpio_tbl_t gpio_tbl[GPIO_MAX_CH] =
    {
      {25, _DEF_OUTPUT, _DEF_HIGH, _DEF_LOW,  _DEF_LOW }, // 0. LED       
      {13, _DEF_OUTPUT, _DEF_HIGH, _DEF_LOW,  _DEF_HIGH}, // 1. MCP2515 CS      
    };



#ifdef _USE_HW_CLI
static void cliGpio(cli_args_t *args);
#endif



bool gpioInit(void)
{
  bool ret = true;


  for (int i=0; i<GPIO_MAX_CH; i++)
  {
    gpio_init(gpio_tbl[i].pin);

    gpioPinMode(i, gpio_tbl[i].mode);
    gpioPinWrite(i, gpio_tbl[i].init_value);
  }

#ifdef _USE_HW_CLI
  cliAdd("gpio", cliGpio);
#endif

  return ret;
}

bool gpioPinMode(uint8_t ch, uint8_t mode)
{
  bool ret = true;


  if (ch >= GPIO_MAX_CH)
  {
    return false;
  }

  switch(mode)
  {
    case _DEF_INPUT:
      gpio_set_pulls(gpio_tbl[ch].pin, false, false);
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_IN);
      break;

    case _DEF_INPUT_PULLUP:
      gpio_set_pulls(gpio_tbl[ch].pin, true, false);
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_IN);
      break;

    case _DEF_INPUT_PULLDOWN:
      gpio_set_pulls(gpio_tbl[ch].pin, false, true);
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_IN);
      break;

    case _DEF_OUTPUT:
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_OUT);
      break;

    case _DEF_OUTPUT_PULLUP:
      gpio_set_pulls(gpio_tbl[ch].pin, true, false);
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_OUT);
      break;

    case _DEF_OUTPUT_PULLDOWN:
      gpio_set_pulls(gpio_tbl[ch].pin, false, true);
      gpio_set_dir(gpio_tbl[ch].pin, GPIO_OUT);
      break;
  }

  return ret;
}

void gpioPinWrite(uint8_t ch, uint8_t value)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  if (value)
  {
    gpio_put(gpio_tbl[ch].pin, gpio_tbl[ch].on_state);
  }
  else
  {
    gpio_put(gpio_tbl[ch].pin, gpio_tbl[ch].off_state);
  }
}

uint8_t gpioPinRead(uint8_t ch)
{
  uint8_t ret = _DEF_LOW;

  if (ch >= GPIO_MAX_CH)
  {
    return false;
  }

  if (gpio_get(gpio_tbl[ch].pin))
  {
    ret = _DEF_HIGH;
  }

  return ret;
}

void gpioPinToggle(uint8_t ch)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  gpio_xor_mask(1<<gpio_tbl[ch].pin);
}





#ifdef _USE_HW_CLI
void cliGpio(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    while(cliKeepLoop())
    {
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        cliPrintf("%d", gpioPinRead(i));
      }
      cliPrintf("\n");
      delay(100);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    while(cliKeepLoop())
    {
      cliPrintf("gpio read %d : %d\n", ch, gpioPinRead(ch));
      delay(100);
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "write") == true)
  {
    uint8_t ch;
    uint8_t data;

    ch   = (uint8_t)args->getData(1);
    data = (uint8_t)args->getData(2);

    gpioPinWrite(ch, data);

    cliPrintf("gpio write %d : %d\n", ch, data);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("gpio show\n");
    cliPrintf("gpio read ch[0~%d]\n", GPIO_MAX_CH-1);
    cliPrintf("gpio write ch[0~%d] 0:1\n", GPIO_MAX_CH-1);
  }
}
#endif


#endif
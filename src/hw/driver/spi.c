/*
 * spi.c
 *
 *  Created on: 2020. 12. 27.
 *      Author: baram
 */


#include "spi.h"
#include "hardware/spi.h"


#ifdef _USE_HW_SPI

typedef struct
{
  bool is_open;
  bool is_tx_done;
  bool is_error;
  uint8_t bit_width;

  void (*func_tx)(void);

  spi_inst_t *h_spi;
} spi_t;



spi_t spi_tbl[SPI_MAX_CH];




bool spiInit(void)
{
  bool ret = true;


  for (int i=0; i<SPI_MAX_CH; i++)
  {
    spi_tbl[i].is_open = false;
    spi_tbl[i].is_tx_done = true;
    spi_tbl[i].is_error = false;
    spi_tbl[i].func_tx = NULL;
    spi_tbl[i].bit_width = 8;
  }

  return ret;
}

bool spiBegin(uint8_t ch)
{
  bool ret = false;
  spi_t *p_spi = &spi_tbl[ch];

  switch(ch)
  {
    case _DEF_SPI1:
      p_spi->h_spi = spi1;

      spi_init(p_spi->h_spi, 10*1000*1000); // 10Mhz
      gpio_set_function(10, GPIO_FUNC_SPI); // SCK
      gpio_set_function(11, GPIO_FUNC_SPI); // MOSI
      gpio_set_function(12, GPIO_FUNC_SPI); // MISO

      p_spi->is_open = true;
      ret = true;
      break;

    case _DEF_SPI2:
      break;
  }

  return ret;
}

void spiSetDataMode(uint8_t ch, uint8_t dataMode)
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return;


  switch( dataMode )
  {
    // CPOL=0, CPHA=0
    case SPI_MODE0:
      break;

    // CPOL=0, CPHA=1
    case SPI_MODE1:
      break;

    // CPOL=1, CPHA=0
    case SPI_MODE2:
      break;

    // CPOL=1, CPHA=1
    case SPI_MODE3:
      break;
  }
}

void spiSetBitWidth(uint8_t ch, uint8_t bit_width)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return;

  p_spi->bit_width = bit_width;
}

uint8_t spiTransfer8(uint8_t ch, uint8_t data)
{
  uint8_t ret;
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return 0;

  spiTransfer(ch, &data, &ret, 1, 10);

  return ret;
}

uint16_t spiTransfer16(uint8_t ch, uint16_t data)
{
  uint8_t tBuf[2];
  uint8_t rBuf[2];
  uint16_t ret;
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return 0;

  if (p_spi->bit_width == 8)
  {
    tBuf[1] = (uint8_t)data;
    tBuf[0] = (uint8_t)(data>>8);
    spiTransfer(ch, &tBuf[0], &rBuf[0], 2, 10);

    ret = rBuf[0];
    ret <<= 8;
    ret += rBuf[1];
  }
  else
  {
    spiTransfer(ch, (uint8_t *)&data, (uint8_t *)&ret, 2, 10);
  }

  return ret;
}

bool spiTransfer(uint8_t ch, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t length, uint32_t timeout)
{
  bool ret = true;
  spi_t  *p_spi = &spi_tbl[ch];
  uint32_t pre_time;
  uint32_t tx_i = 0;
  uint32_t rx_i = 0;

  if (p_spi->is_open == false) return false;

  const size_t fifo_depth = 8;
  size_t rx_remaining = length;
  size_t tx_remaining = length;

  pre_time = millis();
  while (rx_remaining || tx_remaining) 
  {
    if (tx_remaining && spi_is_writable(p_spi->h_spi) && rx_remaining < tx_remaining + fifo_depth) 
    {
      if (tx_buf == NULL)
        spi_get_hw(p_spi->h_spi)->dr = (uint32_t)0xFF;        
      else
        spi_get_hw(p_spi->h_spi)->dr = (uint32_t) tx_buf[tx_i];

      tx_i++;
      --tx_remaining;
    }

    if (rx_remaining && spi_is_readable(p_spi->h_spi)) 
    {
      if (rx_buf == NULL)
        (void)spi_get_hw(p_spi->h_spi)->dr;
      else
        rx_buf[rx_i] = (uint8_t) spi_get_hw(p_spi->h_spi)->dr;
      --rx_remaining;
      rx_i++;
    }

    if (millis()-pre_time >= timeout)
    {
      ret = false;
      break;
    }
  }

  return ret;
}

void spiDmaTxStart(uint8_t spi_ch, uint8_t *p_buf, uint32_t length)
{
  spi_t  *p_spi = &spi_tbl[spi_ch];

  if (p_spi->is_open == false) return;

  p_spi->is_tx_done = false;
}

bool spiDmaTxTransfer(uint8_t ch, void *buf, uint32_t length, uint32_t timeout)
{
  bool ret = true;
  uint32_t t_time;


  spiDmaTxStart(ch, (uint8_t *)buf, length);

  t_time = millis();

  if (timeout == 0) return true;

  while(1)
  {
    if(spiDmaTxIsDone(ch))
    {
      break;
    }
    if((millis()-t_time) > timeout)
    {
      ret = false;
      break;
    }
  }

  return ret;
}

bool spiDmaTxIsDone(uint8_t ch)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false)     return true;

  return p_spi->is_tx_done;
}

void spiAttachTxInterrupt(uint8_t ch, void (*func)())
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false)     return;

  p_spi->func_tx = func;
}


#endif
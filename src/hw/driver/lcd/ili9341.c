/*
 * ili9341.c
 *
 *  Created on: 2020. 3. 26.
 *      Author: HanCheol Cho
 */




#include "lcd/ili9341.h"

#ifdef _USE_HW_ILI9341
#include "gpio.h"
#include "spi.h"





#define _PIN_DEF_RST    4
#define _PIN_DEF_DC     2
#define _PIN_DEF_CS     1


static uint8_t spi_ch = _DEF_SPI1;

static int32_t _width  = ILI9341_LCD_WIDTH;
static int32_t _height = ILI9341_LCD_HEIGHT;

static void (*frameCallBack)(void) = NULL;
volatile static bool  is_write_frame = false;


static void writecommand(uint8_t c);
static void writedata(uint8_t d);
static void ili9341InitRegs(void);
static bool ili9341Reset(void);
static void ili9341FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
static void illi9341SetRotation(uint8_t m);
static bool ili9341SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);
static bool ili9341SetCallBack(void (*p_func)(void));
static void ili9341SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static uint16_t ili9341GetWidth(void);
static uint16_t ili9341GetHeight(void);


static void transferDoneISR(void)
{
  if (is_write_frame == true)
  {
    is_write_frame = false;
    gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

    if (frameCallBack != NULL)
    {
      frameCallBack();
    }
  }
}



bool ili9341Init(void)
{
  bool ret;

  logPrintf("ili9341\n");

  ret = ili9341Reset();

  return ret;
}

bool ili9341InitDriver(lcd_driver_t *p_driver)
{
  p_driver->init = ili9341Init;
  p_driver->reset = ili9341Reset;
  p_driver->setWindow = ili9341SetWindow;
  p_driver->getWidth = ili9341GetWidth;
  p_driver->getHeight = ili9341GetHeight;
  p_driver->setCallBack = ili9341SetCallBack;
  p_driver->sendBuffer = ili9341SendBuffer;
  return true;
}

bool ili9341Reset(void)
{
  spiBegin(spi_ch);
  spiSetDataMode(spi_ch, SPI_MODE0);
  spiSetBaudRate(spi_ch, 500*1000);

  spiAttachTxInterrupt(spi_ch, transferDoneISR);

  gpioPinWrite(_PIN_DEF_DC,  _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS,  _DEF_HIGH);

  gpioPinWrite(_PIN_DEF_RST, _DEF_LOW);
  delay(50);
  gpioPinWrite(_PIN_DEF_RST, _DEF_HIGH);


  ili9341InitRegs();
  illi9341SetRotation(2);

  spiSetBaudRate(spi_ch, 62*1024*1024);
  
  logPrintf("   spi clk : %d Mhz\n", spiGetBaudRate(spi_ch)/1000000);
  return true;
}

uint16_t ili9341GetWidth(void)
{
  return _width;
}

uint16_t ili9341GetHeight(void)
{
  return _height;
}

void writecommand(uint8_t c)
{
  gpioPinWrite(_PIN_DEF_DC, _DEF_LOW);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer8(spi_ch, c);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

void writedata(uint8_t d)
{
  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer8(spi_ch, d);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

void ili9341InitRegs(void)
{  
  writecommand(0xEF);
  writedata(0x03);
  writedata(0x80);
  writedata(0x02);

  writecommand(0xCF);
  writedata(0x00);
  writedata(0XC1);
  writedata(0X30);

  writecommand(0xED);
  writedata(0x64);
  writedata(0x03);
  writedata(0X12);
  writedata(0X81);

  writecommand(0xE8);
  writedata(0x85);
  writedata(0x00);
  writedata(0x78);

  writecommand(0xCB);
  writedata(0x39);
  writedata(0x2C);
  writedata(0x00);
  writedata(0x34);
  writedata(0x02);

  writecommand(0xF7);
  writedata(0x20);

  writecommand(0xEA);
  writedata(0x00);
  writedata(0x00);

  writecommand(ILI9341_PWCTR1);     //Power control
  writedata(0x23);                  //VRH[5:0]

  writecommand(ILI9341_PWCTR2);     //Power control
  writedata(0x10);                  //SAP[2:0];BT[3:0]

  writecommand(ILI9341_VMCTR1);     //VCM control
  writedata(0x3e);
  writedata(0x28);

  writecommand(ILI9341_VMCTR2);     //VCM control2
  writedata(0x86);                  //--

  writecommand(ILI9341_MADCTL);     // Memory Access Control

  writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER); // Rotation 0 (portrait mode)

  writecommand(ILI9341_PIXFMT);
  writedata(0x55);

  writecommand(ILI9341_FRMCTR1);
  writedata(0x00);
  writedata(0x13);                  // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz

  writecommand(ILI9341_DFUNCTR);    // Display Function Control
  writedata(0x08);
  writedata(0x82);
  writedata(0x27);

  writecommand(0xF2);               // 3Gamma Function Disable
  writedata(0x00);

  writecommand(ILI9341_GAMMASET);   //Gamma curve selected
  writedata(0x01);

  writecommand(ILI9341_GMCTRP1);    //Set Gamma
  writedata(0x0F);
  writedata(0x31);
  writedata(0x2B);
  writedata(0x0C);
  writedata(0x0E);
  writedata(0x08);
  writedata(0x4E);
  writedata(0xF1);
  writedata(0x37);
  writedata(0x07);
  writedata(0x10);
  writedata(0x03);
  writedata(0x0E);
  writedata(0x09);
  writedata(0x00);

  writecommand(ILI9341_GMCTRN1);    //Set Gamma
  writedata(0x00);
  writedata(0x0E);
  writedata(0x14);
  writedata(0x03);
  writedata(0x11);
  writedata(0x07);
  writedata(0x31);
  writedata(0xC1);
  writedata(0x48);
  writedata(0x08);
  writedata(0x0F);
  writedata(0x0C);
  writedata(0x31);
  writedata(0x36);
  writedata(0x0F);

  writecommand(ILI9341_SLPOUT);    //Exit Sleep


  delay(120);

  writecommand(ILI9341_DISPON);    //Display on
}


void illi9341SetRotation(uint8_t m)
{
  uint8_t rotation;

  rotation = m % 8; // Limit the range of values to 0-7

  writecommand(TFT_MADCTL);
  switch (rotation)
  {
    case 0:
      writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      //_width  = _init_width;
      //_height = _init_height;
      break;
    case 1:
      writedata(TFT_MAD_COLOR_ORDER);
      //_width  = _init_height;
      //_height = _init_width;
      break;
    case 2:
      writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
      //_width  = _init_width;
      //_height = _init_height;
      break;
    case 3:
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      //_width  = _init_height;
      //_height = _init_width;
      break;
    case 4:
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      //_width  = _init_width;
      //_height = _init_height;
      break;
    case 5:
      writedata(TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      //_width  = _init_height;
      //_height = _init_width;
      break;
    case 6:
      writedata(TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      //_width  = _init_width;
      //_height = _init_height;
      break;
    case 7:
      writedata(TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
      //_width  = _init_height;
      //_height = _init_width;
      break;
  }
}

void ili9341SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  uint8_t buf[8];

  spiSetBitWidth(spi_ch, 8);


  buf[0] = TFT_CASET;
  buf[1] = x0>>8;
  buf[2] = x0>>0;
  buf[3] = x1>>8;
  buf[4] = x1>>0; 
  writecommand(buf[0]);
  writedata(buf[1]);
  writedata(buf[2]);
  writedata(buf[3]);
  writedata(buf[4]);

  buf[0] = TFT_PASET;
  buf[1] = y0>>8;
  buf[2] = y0>>0;
  buf[3] = y1>>8;
  buf[4] = y1>>0;
  writecommand(buf[0]);
  writedata(buf[1]);
  writedata(buf[2]);
  writedata(buf[3]);
  writedata(buf[4]);

  buf[0] = TFT_RAMWR;
  writecommand(buf[0]);
}


void ili9341FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  uint16_t buf[w];

  // Clipping
  if ((x >= _width) || (y >= _height)) return;

  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }

  if ((x + w) > _width)  w = _width  - x;
  if ((y + h) > _height) h = _height - y;

  if ((w < 1) || (h < 1)) return;


  for (int i=0; i<w; i++)
  {
    buf[i] = (color<<8) | (color>>8);
  }

  ili9341SetWindow(x, y, x + w - 1, y + h - 1);

  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  for (int i=0; i<h; i++)
  {
    spiTransfer(spi_ch, (uint8_t *)buf, NULL, w*2, 100);
  }
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

bool ili9341SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms)
{
  is_write_frame = true;

  spiSetBitWidth(spi_ch, 16);

  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiDmaTxTransfer(_DEF_SPI1, (void *)p_data, length, 0);

  return true;
}

bool ili9341SetCallBack(void (*p_func)(void))
{
  frameCallBack = p_func;

  return true;
}

#endif

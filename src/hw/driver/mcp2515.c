/*
 * mcp2515.c
 *
 *  Created on: 2020. 12. 30.
 *      Author: baram
 */


#include "mcp2515.h"
#include "cli.h"
#include "spi.h"
#include "gpio.h"


#ifdef _USE_HW_MCP2515


#define _PIN_DEF_MCP2515_CS_CH0    1


static const uint8_t MCP_SIDH = 0;
static const uint8_t MCP_SIDL = 1;
static const uint8_t MCP_EID8 = 2;
static const uint8_t MCP_EID0 = 3;
static const uint8_t MCP_DLC  = 4;
static const uint8_t MCP_DATA = 5;

static const uint8_t TXB_EXIDE_MASK = 0x08;
static const uint8_t DLC_MASK       = 0x0F;
//static const uint8_t RTR_MASK       = 0x40;

//static const uint8_t RXBnCTRL_RXM_STD    = 0x20;
//static const uint8_t RXBnCTRL_RXM_EXT    = 0x40;
static const uint8_t RXBnCTRL_RXM_STDEXT = 0x00;
static const uint8_t RXBnCTRL_RXM_MASK   = 0x60;
//static const uint8_t RXBnCTRL_RTR        = 0x08;
static const uint8_t RXB0CTRL_BUKT       = 0x04;
static const uint8_t RXB0CTRL_FILHIT_MASK = 0x03;
static const uint8_t RXB1CTRL_FILHIT_MASK = 0x07;
static const uint8_t RXB0CTRL_FILHIT = 0x00;
static const uint8_t RXB1CTRL_FILHIT = 0x01;


const uint8_t spi_ch = _DEF_SPI1;




static bool is_init[HW_MCP2515_MAX_CH];
static McpBaud is_baud[HW_MCP2515_MAX_CH];


uint8_t mcp2515ReadReg(uint8_t ch, uint8_t addr);
bool mcp2515ReadRegs(uint8_t ch, uint8_t addr, uint8_t *p_data, uint16_t length);
bool mcp2515WriteReg(uint8_t ch, uint8_t addr, uint8_t data);
bool mcp2515WriteRegs(uint8_t ch, uint8_t addr, uint8_t *p_data, uint16_t length);
bool mcp2515ModifyReg(uint8_t ch, uint8_t addr, uint8_t mask, uint8_t data);



#ifdef _USE_HW_CLI
static void cliMCP2515(cli_args_t *args);
#endif



static void TransferDoneISR(void)
{

}


void mcp2515csPinWrite(uint8_t ch, bool value)
{
  if ( ch >= HW_MCP2515_MAX_CH )
    return;

  if ( ch==_DEF_CAN1)
  {
    gpioPinWrite(_PIN_DEF_MCP2515_CS_CH0, value);
  }
}


bool mcp2515Init(void)
{
  bool ret = true;

  ret = spiBegin(spi_ch);
  spiAttachTxInterrupt(spi_ch, TransferDoneISR);

  for (int ch=0; ch<HW_MCP2515_MAX_CH; ch++ )
  {
    mcp2515csPinWrite(ch, _DEF_HIGH);

    is_init[ch] = mcp2515Reset(ch);

    if (is_init[ch] == true)
    {
      uint8_t zeros[14];

      memset(zeros, 0, sizeof(zeros));
      mcp2515WriteRegs(ch, MCP_TXB0CTRL, zeros, 14);
      mcp2515WriteRegs(ch, MCP_TXB1CTRL, zeros, 14);
      mcp2515WriteRegs(ch, MCP_TXB2CTRL, zeros, 14);

      mcp2515WriteReg(ch, MCP_RXB0CTRL, 0);
      mcp2515WriteReg(ch, MCP_RXB1CTRL, 0);

      mcp2515WriteReg(ch, MCP_CANINTE, CANINTF_RX0IF | CANINTF_RX1IF | CANINTF_ERRIF | CANINTF_MERRF);

      // receives all valid messages using either Standard or Extended Identifiers that
      // meet filter criteria. RXF0 is applied for RXB0, RXF1 is applied for RXB1
      mcp2515ModifyReg(ch, MCP_RXB0CTRL,
                     RXBnCTRL_RXM_MASK | RXB0CTRL_BUKT | RXB0CTRL_FILHIT_MASK,
                     RXBnCTRL_RXM_STDEXT | RXB0CTRL_BUKT | RXB0CTRL_FILHIT);
      mcp2515ModifyReg(ch, MCP_RXB1CTRL,
                     RXBnCTRL_RXM_MASK | RXB1CTRL_FILHIT_MASK,
                     RXBnCTRL_RXM_STDEXT | RXB1CTRL_FILHIT);

      for (int i=0; i<MCP_FILTER_MAX; i++)
      {
        bool ext;

        if (i == 1)
        {
          ext = true;
        }
        else
        {
          ext = false;
        }
        mcp2515SetFilter(ch, i, ext, 0);
      }
      for (int i=0; i<MCP_MASK_MAX; i++)
      {
        mcp2515SetFilterMask(ch, i, true, 0);
      }
    }
  }

  mcp2515SetMode(_DEF_CAN1, MCP_MODE_NORMAL);
  mcp2515SetBaud(_DEF_CAN1, MCP_BAUD_500K);

#ifdef _USE_HW_CLI
  cliAdd("mcp2515", cliMCP2515);
#endif
  return ret;
}

bool mcp2515Reset(uint8_t ch)
{
  bool ret = false;
  uint8_t buf[1];

  if ( ch >= HW_MCP2515_MAX_CH )
    return ret;

  buf[0] = MCP_INST_RESET;

  mcp2515csPinWrite(ch, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, buf, 1, 10);
  mcp2515csPinWrite(ch, _DEF_HIGH);

  delay(10);

  return ret;
}

bool mcp2515SetMode(uint8_t ch, McpMode mode)
{
  bool ret;
  uint8_t data = 0;
  uint8_t mask = 0;
  uint32_t pre_time;

  data = ((uint8_t)mode)<<5;
  mask = 0x07<<5;

  ret = mcp2515ModifyReg(ch, MCP_CANCTRL, mask, data);

  if (ret == true)
  {
    ret = false;
    pre_time = millis();
    while(millis()-pre_time < 10)
    {
      if ((mcp2515ReadReg(ch, MCP_CANSTAT)&mask) == data)
      {
        ret = true;
        break;
      }
    }
  }

  return ret;
}

McpMode mcp2515GetMode(uint8_t ch)
{
  McpMode ret;
  uint8_t data;

  data = (mcp2515ReadReg(ch, MCP_CANSTAT) >> 5) & 0x07;
  ret = data;

  return ret;
}

bool mcp2515SetBaud(uint8_t ch, McpBaud baud)
{
  bool ret;
  uint8_t cfg1;
  uint8_t cfg2;
  uint8_t cfg3;
  McpMode mode;

  mode = mcp2515GetMode(ch);

  switch(baud)
  {
    case MCP_BAUD_100K:
      cfg1 = 0x04;
      cfg2 = 0x90;
      cfg3 = 0x82;
      break;

    case MCP_BAUD_125K:
      cfg1 = 0x03;
      cfg2 = 0x90;
      cfg3 = 0x82;
      break;

    case MCP_BAUD_250K:
      cfg1 = 0x01;
      cfg2 = 0x90;
      cfg3 = 0x82;
      break;

    case MCP_BAUD_500K:
      cfg1 = 0x00;
      cfg2 = 0x90;
      cfg3 = 0x82;
      break;

    case MCP_BAUD_1000K:
      cfg1 = 0x00;
      cfg2 = 0x80;
      cfg3 = 0x80;
      break;
  }

  mcp2515SetMode(ch, MCP_MODE_CONFIG);

  ret = mcp2515WriteReg(ch, MCP_CNF1, cfg1);
  ret = mcp2515WriteReg(ch, MCP_CNF2, cfg2);
  ret = mcp2515WriteReg(ch, MCP_CNF3, cfg3);
  is_baud[ch] = baud;

  mcp2515SetMode(ch, mode);

  return ret;
}

McpBaud mcp2515GetBaud(uint8_t ch)
{
  return is_baud[ch];
}

uint8_t mcp2515ReadStatus(uint8_t ch)
{
  uint8_t ret = 0;
  uint8_t buf[2];

  buf[0] = MCP_INST_READ_STATUS;
  buf[1] = 0;

  mcp2515csPinWrite(ch, _DEF_LOW);
  if (spiTransfer(spi_ch, buf, buf, 2, 10) == true)
  {
    ret = buf[1];
  }
  mcp2515csPinWrite(ch, _DEF_HIGH);


  return ret;
}

uint8_t mcp2515ReadErrorFlags(uint8_t ch)
{
  return mcp2515ReadReg(ch, MCP_EFLG);
}

void mcp2515PrepareID(uint8_t *buffer, const bool ext, const uint32_t id)
{
  uint16_t canid = (uint16_t)(id & 0x0FFFF);

  if (ext)
  {
    buffer[MCP_EID0] = (uint8_t) (canid & 0xFF);
    buffer[MCP_EID8] = (uint8_t) (canid >> 8);
    canid = (uint16_t)(id >> 16);
    buffer[MCP_SIDL] = (uint8_t) (canid & 0x03);
    buffer[MCP_SIDL] += (uint8_t) ((canid & 0x1C) << 3);
    buffer[MCP_SIDL] |= 0x08;
    buffer[MCP_SIDH] = (uint8_t) (canid >> 5);
  }
  else
  {
    buffer[MCP_SIDH] = (uint8_t) (canid >> 3);
    buffer[MCP_SIDL] = (uint8_t) ((canid & 0x07 ) << 5);
    buffer[MCP_EID0] = 0;
    buffer[MCP_EID8] = 0;
  }
}

bool mcp2515SetFilterMask(uint8_t ch, uint8_t index, const bool ext, const uint32_t data)
{
  bool ret;
  uint8_t buf[4];
  McpMode mode;

  if (index >= MCP_MASK_MAX) return false;


  mcp2515PrepareID(buf, ext, data);

  mode = mcp2515GetMode(ch);
  mcp2515SetMode(ch, MCP_MODE_CONFIG);

  ret = mcp2515WriteRegs(ch, MCP_RXMSIDH(index), buf, 4);

  mcp2515SetMode(ch, mode);

  return ret;
}

bool mcp2515SetFilter(uint8_t ch, uint8_t index, const bool ext, const uint32_t data)
{
  bool ret;
  uint8_t buf[4];
  McpMode mode;
  const uint8_t rxf_addr[MCP_FILTER_MAX] = {0x00, 0x04, 0x08, 0x10, 0x14, 0x18};

  if (index >= MCP_FILTER_MAX) return false;


  mcp2515PrepareID(buf, ext, data);

  mode = mcp2515GetMode(ch);
  mcp2515SetMode(ch, MCP_MODE_CONFIG);

  ret = mcp2515WriteRegs(ch, rxf_addr[index], buf, 4);

  mcp2515SetMode(ch, mode);

  return ret;
}

bool mcp2515SendMsg(uint8_t ch, mcp_msg_t *p_msg)
{
  bool ret = false;
  uint8_t tx_i;
  uint8_t reg;

  for (int i=0; i<3; i++)
  {
    reg = mcp2515ReadReg(ch, MCP_TXBCTRL(i));

    if ((reg & (1<<3)) == 0x00)
    {
      tx_i = i;
      ret = true;
      break;
    }
  }

  if (ret != true)
  {
    return false;
  }

  uint8_t data[13];

  mcp2515PrepareID(data, p_msg->ext, p_msg->id);
  data[MCP_DLC] = p_msg->dlc;

  memcpy(&data[MCP_DATA], p_msg->data, p_msg->dlc);

  ret = mcp2515WriteRegs(ch, MCP_TXBSIDH(tx_i), data, 5 + p_msg->dlc);

  ret = mcp2515ModifyReg(ch, MCP_TXBCTRL(tx_i), (1<<3), (1<<3));
  if (ret == true)
  {
    reg = mcp2515ReadReg(ch, MCP_TXBCTRL(tx_i));

    if (reg & (0x70))
    {
      ret = false;
    }
  }

  return ret;
}

bool mcp2515ReadMsg(uint8_t ch, mcp_msg_t *p_msg)
{
  bool ret = false;
  uint8_t rx_i = 0;
  uint8_t reg;

  reg = mcp2515ReadStatus(ch);

  if (reg & (1<<0))
  {
    rx_i = 0;
  }
  else if (reg & (1<<1))
  {
    rx_i = 1;
  }
  else
  {
    return false;
  }


  uint8_t tbufdata[13];

  mcp2515ReadRegs(ch, MCP_RXBSIDH(rx_i), tbufdata, 13);

  uint32_t id;

  id = (tbufdata[MCP_SIDH]<<3) + (tbufdata[MCP_SIDL]>>5);
  p_msg->ext = false;

  if ( (tbufdata[MCP_SIDL] & TXB_EXIDE_MASK) ==  TXB_EXIDE_MASK )
  {
    id = (id<<2) + (tbufdata[MCP_SIDL] & 0x03);
    id = (id<<8) + tbufdata[MCP_EID8];
    id = (id<<8) + tbufdata[MCP_EID0];
    p_msg->ext = true;
  }

  uint8_t dlc = (tbufdata[MCP_DLC] & DLC_MASK);
  if (dlc > 8)
  {
    return false;
  }

  p_msg->id = id;
  p_msg->dlc = dlc;

  for (int i=0; i<dlc; i++)
  {
    p_msg->data[i] = tbufdata[5+i];
  }

  ret = mcp2515ModifyReg(ch, MCP_CANINTF, 1<<rx_i, 0);

  return ret;
}

uint8_t mcp2515ReadReg(uint8_t ch, uint8_t addr)
{
  uint8_t ret = 0;
  uint8_t buf[3];


  buf[0] = MCP_INST_READ;
  buf[1] = addr;

  mcp2515csPinWrite(ch, _DEF_LOW);
  if (spiTransfer(spi_ch, buf, buf, 3, 10) == true)
  {
    ret = buf[2];
  }
  mcp2515csPinWrite(ch, _DEF_HIGH);
  return ret;
}

bool mcp2515ReadRegs(uint8_t ch, uint8_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret;
  uint8_t buf[2];


  buf[0] = MCP_INST_READ;
  buf[1] = addr;

  mcp2515csPinWrite(ch, _DEF_LOW);

  spiTransfer(spi_ch, buf, buf, 2, 10);

  ret = spiTransfer(spi_ch, p_data, p_data, length, 10);

  mcp2515csPinWrite(ch, _DEF_HIGH);

  return ret;
}

bool mcp2515WriteReg(uint8_t ch, uint8_t addr, uint8_t data)
{
  bool ret = 0;
  uint8_t buf[3];


  buf[0] = MCP_INST_WRITE;
  buf[1] = addr;
  buf[2] = data;

  mcp2515csPinWrite(ch, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, NULL, 3, 10);
  mcp2515csPinWrite(ch, _DEF_HIGH);
  return ret;
}

bool mcp2515WriteRegs(uint8_t ch, uint8_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret;
  uint8_t buf[2];


  buf[0] = MCP_INST_WRITE;
  buf[1] = addr;

  mcp2515csPinWrite(ch, _DEF_LOW);

  spiTransfer(spi_ch, buf, buf, 2, 10);

  ret = spiTransfer(spi_ch, p_data, NULL, length, 10);

  mcp2515csPinWrite(ch, _DEF_HIGH);

  return ret;
}

bool mcp2515ModifyReg(uint8_t ch, uint8_t addr, uint8_t mask, uint8_t data)
{
  bool ret = 0;
  uint8_t buf[4];


  buf[0] = MCP_INST_BIT_MODIFY;
  buf[1] = addr;
  buf[2] = mask;
  buf[3] = data;

  mcp2515csPinWrite(ch, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, NULL, 4, 10);
  mcp2515csPinWrite(ch, _DEF_HIGH);
  return ret;
}

uint16_t mcp2515GetRxErrCount(uint8_t ch)
{
  uint16_t ret;

  ret = mcp2515ReadReg(ch, MCP_REC);
  return ret;
}

uint16_t mcp2515GetTxErrCount(uint8_t ch)
{
  uint16_t ret;

  ret = mcp2515ReadReg(ch, MCP_TEC);
  return ret;
}

#ifdef _USE_HW_CLI
void mcp2515Info(uint8_t ch)
{
  uint8_t reg;
  uint8_t reg_bits;

  reg = mcp2515ReadReg(ch, 0x0E);
  reg_bits = (reg>>5) & 0x07;

  cliPrintf("is_init[ch%d]\t: %d\n", ch, is_init[ch]);
  cliPrintf("Operation Mode \t: ");
  if (reg_bits == 0x00 ) cliPrintf("Nomal");
  if (reg_bits == 0x01 ) cliPrintf("Sleep");
  if (reg_bits == 0x02 ) cliPrintf("Loopback");
  if (reg_bits == 0x03 ) cliPrintf("Listen-Only");
  if (reg_bits == 0x04 ) cliPrintf("Configuration");
  cliPrintf("\n");

  uint32_t Fosc;
  uint32_t BRP;
  uint32_t Tq;
  uint32_t SJW;
  uint32_t SyncSeg = 1;
  uint32_t PropSeg;
  uint32_t PhaseSeg1;
  uint32_t PhaseSeg2;
  uint32_t Tbit;
  uint32_t NBR;

  Fosc = 8;
  BRP = ((mcp2515ReadReg(ch, 0x2A) >> 0) & 0x3F);
  Tq  = 2*(BRP+1)*1000 / 8;
  SJW = ((mcp2515ReadReg(ch, 0x2A) >> 6) & 0x03);

  cliPrintf("Fosc \t\t: %dMhz\n", Fosc);
  cliPrintf("BRP  \t\t: %d\n", BRP);
  cliPrintf("Tq   \t\t: %d ns, %d Mhz\n", Tq, 1000/Tq);
  cliPrintf("SJW  \t\t: %d Tq\n", SJW);

  PropSeg   = ((mcp2515ReadReg(ch, 0x29) >> 0) & 0x07) + 1;
  PhaseSeg1 = ((mcp2515ReadReg(ch, 0x29) >> 3) & 0x07) + 1;
  PhaseSeg2 = ((mcp2515ReadReg(ch, 0x28) >> 0) & 0x07) + 1;
  Tbit      = SyncSeg + PropSeg + PhaseSeg1 + PhaseSeg2;
  NBR       = 1000000 / (Tbit*Tq);

  cliPrintf("SyncSeg        \t: %d Tq\n", SyncSeg);
  cliPrintf("PropSeg        \t: %d Tq\n", PropSeg);
  cliPrintf("PhaseSeg1(PS1) \t: %d Tq\n", PhaseSeg1);
  cliPrintf("PhaseSeg2(PS2) \t: %d Tq\n", PhaseSeg2);
  cliPrintf("Tbit           \t: %d Tq, %d ns\n", Tbit, Tbit * Tq);
  cliPrintf("Sample Point   \t: %d%% \n", (SyncSeg+PropSeg+PhaseSeg1) * 100 / Tbit);
  cliPrintf("NBR            \t: %d Kbps\n", NBR);

}

void cliMCP2515(cli_args_t *args)
{
  bool ret = false;
  uint8_t ch;

  if (args->argc == 2 && args->isStr(1, "info") == true)
  {
    ch = (uint8_t)args->getData(0);
    mcp2515Info(ch);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(1, "reg_info") == true)
  {
    ch = (uint8_t)args->getData(0);
    cliPrintf("BFPCTRL    0x%02X : 0x%02X\n", 0x0C, mcp2515ReadReg(ch, 0x0C));
    cliPrintf("TXRTSCTRL  0x%02X : 0x%02X\n", 0x0D, mcp2515ReadReg(ch, 0x0D));
    cliPrintf("CANSTAT    0x%02X : 0x%02X\n", 0x0E, mcp2515ReadReg(ch, 0x0E));
    cliPrintf("CANCTRL    0x%02X : 0x%02X\n", 0x0F, mcp2515ReadReg(ch, 0x0F));
    cliPrintf("TEC        0x%02X : 0x%02X\n", 0x1C, mcp2515ReadReg(ch, 0x1C));
    cliPrintf("REC        0x%02X : 0x%02X\n", 0x1D, mcp2515ReadReg(ch, 0x1D));
    cliPrintf("CNF3       0x%02X : 0x%02X\n", 0x28, mcp2515ReadReg(ch, 0x28));
    cliPrintf("CNF2       0x%02X : 0x%02X\n", 0x29, mcp2515ReadReg(ch, 0x29));
    cliPrintf("CNF1       0x%02X : 0x%02X\n", 0x2A, mcp2515ReadReg(ch, 0x2A));
    cliPrintf("CANINTE    0x%02X : 0x%02X\n", 0x2B, mcp2515ReadReg(ch, 0x2B));
    cliPrintf("CANINTF    0x%02X : 0x%02X\n", 0x2C, mcp2515ReadReg(ch, 0x2C));
    cliPrintf("EFLG       0x%02X : 0x%02X\n", 0x2D, mcp2515ReadReg(ch, 0x2D));
    cliPrintf("TXB0CTRL   0x%02X : 0x%02X\n", 0x30, mcp2515ReadReg(ch, 0x30));
    cliPrintf("TXB1CTRL   0x%02X : 0x%02X\n", 0x40, mcp2515ReadReg(ch, 0x40));
    cliPrintf("TXB2CTRL   0x%02X : 0x%02X\n", 0x50, mcp2515ReadReg(ch, 0x50));
    cliPrintf("RXB0CTRL   0x%02X : 0x%02X\n", 0x60, mcp2515ReadReg(ch, 0x60));
    cliPrintf("RXB1CTRL   0x%02X : 0x%02X\n", 0x70, mcp2515ReadReg(ch, 0x70));

    uint32_t pre_time;

    pre_time = millis();
    for (int i=0; i<1000; i++)
    {
      mcp2515ReadReg(ch, 0x2A);
    }
    cliPrintf("%d ms\n", millis()-pre_time);

    ret = true;
  }

  if (args->argc == 4 && args->isStr(1, "read_reg") == true)
  {
    uint8_t  addr;
    uint16_t length;
    uint8_t buf[2];

    ch     = (uint8_t)args->getData(0);
    addr   = (uint8_t)args->getData(2);
    length = (uint16_t)args->getData(3);

    cliPrintf("<mcp2515 ch%d read register>\n", ch);
    for (int i=0; i<length; i++)
    {
      if (mcp2515ReadRegs(ch, addr+i, buf, 1) == true)
      {
        cliPrintf("0x%02X : 0x%02X\n", addr+i, buf[0]);
      }
      else
      {
        cliPrintf("spi fail\n");
        break;
      }

      ret = true;
    }
  }

  if (args->argc == 3 && args->isStr(1, "set_baud") == true)
  {
    bool update = false;
    ch = (uint8_t)args->getData(0);

    if (args->isStr(2, "100k"))
    {
      mcp2515SetBaud(ch, MCP_BAUD_100K);
      update = true;
    }
    else if (args->isStr(2, "125k"))
    {
      mcp2515SetBaud(ch, MCP_BAUD_125K);
      update = true;
    }
    else if (args->isStr(2, "250k"))
    {
      mcp2515SetBaud(ch, MCP_BAUD_250K);
      update = true;
    }
    else if (args->isStr(2, "500k"))
    {
      mcp2515SetBaud(ch, MCP_BAUD_500K);
      update = true;
    }
    else if (args->isStr(2, "1000k"))
    {
      mcp2515SetBaud(ch, MCP_BAUD_1000K);
      update = true;
    }

    if (update == true)
    {
      cliPrintf("ch%d Baud %s OK\n", ch, args->getStr(2));
    }
    else
    {
      cliPrintf("ch%d Wrong Baud\n", ch);
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(1, "set_mode") == true)
  {
    bool update = false;
    ch = (uint8_t)args->getData(0);

    if (args->isStr(2, "normal"))
    {
      update = mcp2515SetMode(ch, MCP_MODE_NORMAL);
    }
    else if (args->isStr(2, "loopback"))
    {
      update = mcp2515SetMode(ch, MCP_MODE_LOOPBACK);
    }
    else if (args->isStr(2, "listen"))
    {
      update = mcp2515SetMode(ch, MCP_MODE_LISTEN);
    }
    else if (args->isStr(2, "config"))
    {
      update = mcp2515SetMode(ch, MCP_MODE_CONFIG);
    }

    if (update == true)
    {
      cliPrintf("ch%d Mode %s OK\n", ch, args->getStr(2));
    }
    else
    {
      cliPrintf("ch%d Wrong Mode\n", ch);
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(1, "test") == true)
  {
    uint8_t rx_data;
    mcp_msg_t rx_msg;
    uint8_t cnt = 0;


    ch = (uint8_t)args->getData(0);


    while(1)
    {
      if (mcp2515ReadMsg(ch, &rx_msg) == true)
      {
        cliPrintf("ch: %d, id: %03X, dlc: %d, ext: %d, ",
                  ch,
                  rx_msg.id,
                  rx_msg.dlc,
                  rx_msg.ext);

        for (int i=0; i<rx_msg.dlc; i++)
        {
          cliPrintf("%02X ", rx_msg.data[i]);
        }
        cliPrintf("\n");
      }

      if (cliAvailable() > 0)
      {
        rx_data = cliRead();

        if (rx_data <= 0x20)
        {
          break;
        }

        uint8_t rx_ch = 0;

        mcp_msg_t msg;
        msg.id  = 0x123;
        msg.ext = true;
        msg.dlc = 8;
        cnt++;
        msg.data[0] = cnt<<0;
        msg.data[1] = cnt<<1;
        msg.data[2] = cnt<<2;
        msg.data[3] = cnt<<3;
        msg.data[4] = (cnt<<4)+(cnt&0x0F<<0);
        msg.data[5] = (cnt<<5)+(cnt&0x0F<<1);
        msg.data[6] = (cnt<<6)+(cnt&0x0F<<2);
        msg.data[7] = (cnt<<7)+(cnt&0x0F<<3);

        cliPrintf("\n");
        if (mcp2515SendMsg(rx_ch, &msg) == true)
        {
          cliPrintf("ch%d SendMsg: OK\n", rx_ch);
        }
        else
        {
          cliPrintf("ch%d SendMsg: Fail\n", rx_ch);
        }

        uint8_t status;

        cliPrintf("ch%d Status : 0b", rx_ch);

        status = mcp2515ReadStatus(rx_ch);
        for (int i=0; i<8; i++)
        {
          if (status & 0x80)
          {
            cliPrintf("1");
          }
          else
          {
            cliPrintf("0");
          }
          status <<= 1;
        }
        cliPrintf("\n");
        cliPrintf("ch%d ErrFlag: 0x%02X", rx_ch, mcp2515ReadErrorFlags(rx_ch));
        cliPrintf("\n");
      }
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "reset") == true)
  {
    mcp2515Init();

    for (int ch=0; ch<HW_MCP2515_MAX_CH; ch++ )
    {
      if (is_init[ch] == true)
      {
        cliPrintf("ch%d reset OK\n", ch);
      }
      else
      {
        cliPrintf("ch%d reset Fail\n", ch);
      }
    }

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("mcp2515 ch[0~%d] info\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 ch[0~%d] reg_info\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 ch[0~%d] read_reg addr length\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 ch[0~%d] set_baud 100k:125k:250k:500k:1000k\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 ch[0~%d] set_mode normal:loopback:listen:config\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 ch[0~%d] test\n", MCP2515_MAX_CH-1);
    cliPrintf("mcp2515 reset\n");
  }
}
#endif

#endif
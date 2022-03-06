/*
 * cmd_can.c
 *
 *  Created on: 2021. 12. 10.
 *      Author: hancheol
 */




#include "cmd_can.h"
#ifdef _USE_HW_CLI
#include "cli.h"
#endif


#ifdef _USE_HW_CMD_CAN



#define CMD_CAN_STX                 0xF0
#define CMD_CAN_ETX                 0xFE


typedef enum
{
  CMD_CAN_STATE_WAIT_STX,
  CMD_CAN_STATE_WAIT_TYPE,
  CMD_CAN_STATE_WAIT_CMD,
  CMD_CAN_STATE_WAIT_LENGTH_L,
  CMD_CAN_STATE_WAIT_LENGTH_H,
  CMD_CAN_STATE_WAIT_DATA,
  CMD_CAN_STATE_WAIT_CRC,
  CMD_CAN_STATE_WAIT_ETX
} CmdCanState_t;





void cmdCanInit(cmd_can_t *p_cmd, cmd_can_driver_t *p_driver)
{
  p_cmd->is_open  = false;
  p_cmd->p_driver = p_driver;
  p_cmd->state    = CMD_CAN_STATE_WAIT_STX;

  p_cmd->packet.data    = &p_cmd->rx_packet.buffer[CMD_CAN_STATE_WAIT_DATA];
  p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[CMD_CAN_STATE_WAIT_DATA];
  p_cmd->tx_packet.data = &p_cmd->tx_packet.buffer[CMD_CAN_STATE_WAIT_DATA];

  p_cmd->is_init = true;
}

bool cmdCanOpen(cmd_can_t *p_cmd)
{
  p_cmd->is_open = true;
  p_cmd->state = CMD_CAN_STATE_WAIT_STX;
  p_cmd->pre_time = millis();

  return true;
}

bool cmdCanClose(cmd_can_t *p_cmd)
{
  p_cmd->is_open = false;
  return true;
}

bool cmdCanIsBusy(cmd_can_t *p_cmd)
{
  return p_cmd->p_driver->available();
}

bool cmdCanReceivePacket(cmd_can_t *p_cmd)
{
  bool ret = false;
  uint8_t rx_data;
  cmd_can_driver_t *p_driver = p_cmd->p_driver;


  if (p_cmd->is_open != true) return false;


  while(ret != true && p_driver->available() > 0)
  {
    rx_data = p_driver->read();

    if (millis()-p_cmd->pre_time >= 100)
    {
      p_cmd->state = CMD_CAN_STATE_WAIT_STX;
    }
    p_cmd->pre_time = millis();

    switch(p_cmd->state)
    {
      case CMD_CAN_STATE_WAIT_STX:
        if (rx_data == CMD_CAN_STX)
        {
          p_cmd->state = CMD_CAN_STATE_WAIT_TYPE;
          p_cmd->rx_packet.crc = 0 ^ rx_data;
        }
        break;

      case CMD_CAN_STATE_WAIT_TYPE:
        p_cmd->rx_packet.type = (CmdCanType_t)rx_data;
        p_cmd->rx_packet.crc ^= rx_data;
        p_cmd->state = CMD_CAN_STATE_WAIT_CMD;
        break;

      case CMD_CAN_STATE_WAIT_CMD:
        p_cmd->rx_packet.cmd  = (CmdCanCmd_t)rx_data;
        p_cmd->rx_packet.crc ^= rx_data;
        p_cmd->state = CMD_CAN_STATE_WAIT_LENGTH_L;
        break;

      case CMD_CAN_STATE_WAIT_LENGTH_L:
        p_cmd->rx_packet.length = rx_data;
        p_cmd->rx_packet.crc   ^= rx_data;
        p_cmd->state = CMD_CAN_STATE_WAIT_LENGTH_H;
        break;

      case CMD_CAN_STATE_WAIT_LENGTH_H:
        p_cmd->rx_packet.length |= (rx_data << 8);
        p_cmd->rx_packet.crc    ^= rx_data;

        if (p_cmd->rx_packet.length > 0)
        {
          p_cmd->index = 0;
          p_cmd->state = CMD_CAN_STATE_WAIT_DATA;
        }
        else
        {
          p_cmd->state = CMD_CAN_STATE_WAIT_CRC;
        }
        break;

      case CMD_CAN_STATE_WAIT_DATA:
        p_cmd->rx_packet.data[p_cmd->index] = rx_data;
        p_cmd->rx_packet.crc ^= rx_data;
        p_cmd->index++;
        if (p_cmd->index == p_cmd->rx_packet.length)
        {
          p_cmd->state = CMD_CAN_STATE_WAIT_CRC;
        }
        break;

      case CMD_CAN_STATE_WAIT_CRC:
        p_cmd->rx_packet.crc_recv = rx_data;
        p_cmd->state = CMD_CAN_STATE_WAIT_ETX;
        break;

      case CMD_CAN_STATE_WAIT_ETX:
        if (rx_data == CMD_CAN_ETX)
        {
          if (p_cmd->rx_packet.crc == p_cmd->rx_packet.crc_recv)
          {
            ret = true;
          }
        }
        p_cmd->state = CMD_CAN_STATE_WAIT_STX;
        break;
    }
  }

  return ret;
}

bool cmdCanSend(cmd_can_t *p_cmd, CmdCanType_t type, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length)
{
  uint32_t index;
  cmd_can_driver_t *p_driver = p_cmd->p_driver;
  uint32_t data_len;
  uint32_t wr_len;

  if (p_cmd->is_open != true) return false;

  data_len = length;

  index = 0;
  p_cmd->tx_packet.buffer[index++] = CMD_CAN_STX;
  p_cmd->tx_packet.buffer[index++] = type;
  p_cmd->tx_packet.buffer[index++] = cmd;
  p_cmd->tx_packet.buffer[index++] = (data_len >> 0) & 0xFF;
  p_cmd->tx_packet.buffer[index++] = (data_len >> 8) & 0xFF;

  for (int i=0; i<data_len; i++)
  {
    p_cmd->tx_packet.buffer[index++] = p_data[i];
  }

  uint8_t crc = 0;
  for (int i=0; i<index; i++)
  {
    crc ^= p_cmd->tx_packet.buffer[i];
  }
  p_cmd->tx_packet.buffer[index++] = crc;
  p_cmd->tx_packet.buffer[index++] = CMD_CAN_ETX;

  wr_len = p_driver->write(p_cmd->tx_packet.buffer, index);

  if (wr_len == index)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool cmdCanSendType(cmd_can_t *p_cmd, CmdCanType_t type, uint8_t *p_data, uint32_t length)
{
  bool ret;

  ret = cmdCanSend(p_cmd, type, PKT_CMD_EMPTY, p_data, length);
  return ret;
}

bool cmdCanSendCmd(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length)
{
  bool ret;

  ret = cmdCanSend(p_cmd, PKT_TYPE_CMD, cmd, p_data, length);
  return ret;
}

bool cmdCanSendResp(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length)
{
  bool ret;

  ret = cmdCanSend(p_cmd, PKT_TYPE_RESP, cmd, p_data, length);
  return ret;
}

bool cmdCanSendCmdRxResp(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout)
{
  bool  ret = false;
  uint32_t time_pre;


  cmdCanSendCmd(p_cmd, cmd, p_data, length);

  time_pre = millis();

  while(1)
  {
    if (cmdCanReceivePacket(p_cmd) == true)
    {
      ret = true;
      break;
    }

    if (millis()-time_pre >= timeout)
    {
      break;
    }
  }

  return ret;
}



#ifdef _USE_HW_CLI

void cliCmdCan(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "show"))
  {
    while(cliKeepLoop())
    {
      delay(100);
    }

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("cmdcan show\n");
  }
}
#endif



#endif

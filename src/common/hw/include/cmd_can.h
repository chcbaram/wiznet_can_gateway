/*
 * can.h
 *
 *  Created on: 2021. 6. 22.
 *      Author: baram
 */

#ifndef CMD_CAN_H_
#define CMD_CAN_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_CMD_CAN


#define CMD_CAN_MAX_DATA_LENGTH   HW_CMD_CAN_MAX_DATA_LENGTH


typedef enum CmdCanType
{
  PKT_TYPE_CAN        = 0x00,
  PKT_TYPE_EVENT      = 0x01,
  PKT_TYPE_LOG        = 0x02,
  PKT_TYPE_PING       = 0x03,
  PKT_TYPE_CMD        = 0x04,
  PKT_TYPE_RESP       = 0x05,
} CmdCanType_t;


typedef enum CmdCanCmd
{
  PKT_CMD_EMPTY = 0x00,
  PKT_CMD_PING  = 0x01,
} CmdCanCmd_t;


typedef struct
{
  CmdCanType_t  type;
  CmdCanCmd_t   cmd;
  uint16_t      length;
  uint8_t       crc;
  uint8_t       crc_recv;
  uint8_t       buffer[CMD_CAN_MAX_DATA_LENGTH+8];
  uint8_t      *data;
} cmd_can_packet_t;


typedef struct 
{
  uint32_t (*available)(void);
  bool     (*flush)(void);
  uint8_t  (*read)(void);
  uint32_t (*write)(uint8_t *p_data, uint32_t length);  
} cmd_can_driver_t;


typedef struct
{
  bool      is_init;
  bool      is_open;
  
  uint8_t   state;
  uint32_t  pre_time;
  uint32_t  index;
  uint8_t   error;

  cmd_can_driver_t *p_driver;

  cmd_can_packet_t  packet;
  cmd_can_packet_t  rx_packet;
  cmd_can_packet_t  tx_packet;
} cmd_can_t;


void cmdCanInit(cmd_can_t *p_cmd, cmd_can_driver_t *p_driver);
bool cmdCanOpen(cmd_can_t *p_cmd);
bool cmdCanClose(cmd_can_t *p_cmd);
bool cmdCanIsBusy(cmd_can_t *p_cmd);
bool cmdCanReceivePacket(cmd_can_t *p_cmd);

bool cmdCanSend(cmd_can_t *p_cmd, CmdCanType_t type, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length);
bool cmdCanSendType(cmd_can_t *p_cmd, CmdCanType_t type, uint8_t *p_data, uint32_t length);
bool cmdCanSendCmd(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length);
bool cmdCanSendResp(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length);
bool cmdCanSendCmdRxResp(cmd_can_t *p_cmd, CmdCanCmd_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout);


#endif

#ifdef __cplusplus
 }
#endif


#endif 
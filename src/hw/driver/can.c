/*
 * can.c
 *
 *  Created on: 2021. 6. 22.
 *      Author: baram
 */


#include "can.h"
#include "qbuffer.h"
#include "cli.h"
#include "mcp2515.h"

#ifdef _USE_HW_CAN



const uint32_t dlc_len_tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};



typedef struct
{
  bool is_init;
  bool is_open;

  uint32_t err_code;
  uint8_t  state;
  uint32_t recovery_cnt;

  uint32_t q_rx_full_cnt;
  uint32_t q_tx_full_cnt;
  uint32_t fifo_full_cnt;
  uint32_t fifo_lost_cnt;

  uint32_t fifo_idx;
  uint32_t enable_int;
  can_mode_t  mode;
  can_frame_t frame;
  can_baud_t  baud;
  can_baud_t  baud_data;

  uint8_t hfdcan;
  bool (*handler)(can_msg_t *arg);

  qbuffer_t q_msg;
  can_msg_t can_msg[CAN_MSG_RX_BUF_MAX];
} can_tbl_t;

static can_tbl_t can_tbl[CAN_MAX_CH];

static volatile uint32_t err_int_cnt = 0;


#ifdef _USE_HW_CLI
static void cliCan(cli_args_t *args);
#endif

static void canErrUpdate(uint8_t ch);
static void canRxFifoUpdate(uint8_t ch);




bool canInit(void)
{
  bool ret = true;

  uint8_t i;


  for(i = 0; i < CAN_MAX_CH; i++)
  {
    can_tbl[i].is_init  = true;
    can_tbl[i].is_open  = false;
    can_tbl[i].err_code = CAN_ERR_NONE;
    can_tbl[i].state    = 0;
    can_tbl[i].recovery_cnt = 0;

    can_tbl[i].q_rx_full_cnt = 0;
    can_tbl[i].q_tx_full_cnt = 0;
    can_tbl[i].fifo_full_cnt = 0;
    can_tbl[i].fifo_lost_cnt = 0;

    qbufferCreateBySize(&can_tbl[i].q_msg, (uint8_t *)&can_tbl[i].can_msg[0], sizeof(can_msg_t), CAN_MSG_RX_BUF_MAX);
  }

  mcp2515Init();

#ifdef _USE_HW_CLI
  cliAdd("can", cliCan);
#endif
  return ret;
}

bool canOpen(uint8_t ch, can_mode_t mode, can_frame_t frame, can_baud_t baud, can_baud_t baud_data)
{
  bool ret = true;

  if (ch >= CAN_MAX_CH) return false;

  switch(ch)
  {
    case _DEF_CAN1:
      can_tbl[ch].hfdcan                = 0;
      can_tbl[ch].mode                  = mode;
      can_tbl[ch].frame                 = frame;
      can_tbl[ch].baud                  = baud;
      can_tbl[ch].baud_data             = baud_data;
      can_tbl[ch].fifo_idx              = 0;
      can_tbl[ch].enable_int            = 0;

      mcp2515SetBaud(can_tbl[ch].hfdcan, (McpBaud)baud);
      ret = true;
      break;
  }

  if (ret != true)
  {
    return false;
  }


  canConfigFilter(ch, 0, CAN_STD, 0x0000, 0x0000);
  canConfigFilter(ch, 0, CAN_EXT, 0x0000, 0x0000);


  can_tbl[ch].is_open = true;

  return ret;
}

void canClose(uint8_t ch)
{

}

bool canConfigFilter(uint8_t ch, uint8_t index, can_id_type_t id_type, uint32_t id, uint32_t id_mask)
{
  bool ret = false;


  if (ch >= CAN_MAX_CH) return false;


  if (id_type == CAN_STD)
  {    
  }
  else
  {   
  }

  return ret;
}

uint32_t canMsgAvailable(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return qbufferAvailable(&can_tbl[ch].q_msg);
}

bool canMsgInit(can_msg_t *p_msg, can_frame_t frame, can_id_type_t  id_type, can_dlc_t dlc)
{
  p_msg->frame   = frame;
  p_msg->id_type = id_type;
  p_msg->dlc     = dlc;
  p_msg->length  = dlc_len_tbl[dlc];
  return true;
}

bool canMsgWrite(uint8_t ch, can_msg_t *p_msg, uint32_t timeout)
{
  mcp_msg_t msg;
  bool ret = true;


  if(ch > CAN_MAX_CH) return false;

  if (can_tbl[ch].err_code & CAN_ERR_BUS_OFF) return false;


  switch(p_msg->id_type)
  {
    case CAN_STD :
      msg.ext = false;
      break;

    case CAN_EXT :
      msg.ext = true;
      break;
  }

  switch(p_msg->frame)
  {
    case CAN_CLASSIC:
      break;

    case CAN_FD_NO_BRS:
      break;

    case CAN_FD_BRS:
      break;
  }
  
  msg.id  = p_msg->id;
  msg.ext = true;
  msg.dlc = dlc_len_tbl[p_msg->dlc];

  memcpy(msg.data, p_msg->data, dlc_len_tbl[p_msg->dlc]);

  if (mcp2515SendMsg(can_tbl[ch].hfdcan, &msg) == true)
  {
    ret = true;
  }
  else
  {
    ret = false;
  }

  return ret;
}

bool canMsgRead(uint8_t ch, can_msg_t *p_msg)
{
  bool ret = true;


  if(ch > CAN_MAX_CH) return 0;

  ret = qbufferRead(&can_tbl[ch].q_msg, (uint8_t *)p_msg, 1);

  return ret;
}

uint16_t canGetRxErrCount(uint8_t ch)
{
  uint16_t ret = 0;

  if(ch > CAN_MAX_CH) return 0;

  ret = mcp2515GetRxErrCount(ch);
  return ret;
}

uint16_t canGetTxErrCount(uint8_t ch)
{
  uint16_t ret = 0;

  if(ch > CAN_MAX_CH) return 0;

  ret = mcp2515GetTxErrCount(ch);
  return ret;
}

uint32_t canGetError(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return mcp2515ReadErrorFlags(can_tbl[ch].hfdcan);
}

uint32_t canGetState(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return 0;
}

void canAttachRxInterrupt(uint8_t ch, bool (*handler)(can_msg_t *arg))
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = handler;
}

void canDetachRxInterrupt(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = NULL;
}

void canRecovery(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].recovery_cnt++;
}

bool canUpdate(void)
{
  bool ret = false;
  can_tbl_t *p_can;


  for (int i=0; i<CAN_MAX_CH; i++)
  {
    p_can = &can_tbl[i];

    canErrUpdate(i);
    canRxFifoUpdate(i);

    switch(p_can->state)
    {
      case 0:
        if (p_can->err_code & CAN_ERR_BUS_OFF)
        {
          canRecovery(i);
          p_can->state = 1;
          ret = true;
        }
        break;

      case 1:
        if ((p_can->err_code & CAN_ERR_BUS_OFF) == 0)
        {
          p_can->state = 0;
        }
        break;
    }
  }

  return ret;
}

void canRxFifoUpdate(uint8_t ch)
{
  can_msg_t *rx_buf;
  mcp_msg_t rx_msg;

  if (mcp2515ReadMsg(can_tbl[ch].hfdcan, &rx_msg) != true)
  {
    return;
  }

  rx_buf  = (can_msg_t *)qbufferPeekWrite(&can_tbl[ch].q_msg);

  rx_buf->id = rx_msg.id;

  if(rx_msg.ext != true)
    rx_buf->id_type = CAN_STD;
  else
    rx_buf->id_type = CAN_EXT;

  rx_buf->length = dlc_len_tbl[rx_msg.dlc];
  rx_buf->frame = CAN_CLASSIC;

  if (qbufferWrite(&can_tbl[ch].q_msg, NULL, 1) != true)
  {
    can_tbl[ch].q_rx_full_cnt++;
  }

  if( can_tbl[ch].handler != NULL )
  {
    if ((*can_tbl[ch].handler)((void *)rx_buf) == true)
    {
      qbufferRead(&can_tbl[ch].q_msg, NULL, 1);
    }
  }
}

void canErrClear(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].err_code = CAN_ERR_NONE;
}

void canErrPrint(uint8_t ch)
{
  uint32_t err_code;


  if(ch > CAN_MAX_CH) return;

  err_code = can_tbl[ch].err_code;

  if (err_code & CAN_ERR_PASSIVE) logPrintf("  ERR : CAN_ERR_PASSIVE\n");
  if (err_code & CAN_ERR_WARNING) logPrintf("  ERR : CAN_ERR_WARNING\n");
  if (err_code & CAN_ERR_BUS_OFF) logPrintf("  ERR : CAN_ERR_BUS_OFF\n");
}

void canErrUpdate(uint8_t ch)
{
  uint8_t protocol_status;


  protocol_status = mcp2515ReadErrorFlags(can_tbl[ch].hfdcan);

  if (protocol_status & (1<<3) || protocol_status & (1<<4))
  {
    can_tbl[ch].err_code |= CAN_ERR_PASSIVE;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_PASSIVE;
  }

  if (protocol_status & (1<<0))
  {
    can_tbl[ch].err_code |= CAN_ERR_WARNING;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_WARNING;
  }

  if (protocol_status & (1<<5))
  {
    can_tbl[ch].err_code |= CAN_ERR_BUS_OFF;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_BUS_OFF;
  }

  if (protocol_status & (1<<6))
  {
    can_tbl[ch].fifo_full_cnt++;
  }
}



#ifdef _USE_HW_CLI
void cliCan(cli_args_t *args)
{
  bool ret = false;



  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<CAN_MAX_CH; i++)
    {
      cliPrintf("is_open       : %d\n", can_tbl[i].is_open);

      cliPrintf("q_rx_full_cnt : %d\n", can_tbl[i].q_rx_full_cnt);
      cliPrintf("q_tx_full_cnt : %d\n", can_tbl[i].q_tx_full_cnt);
      cliPrintf("fifo_full_cnt : %d\n", can_tbl[i].fifo_full_cnt);
      cliPrintf("fifo_lost_cnt : %d\n", can_tbl[i].fifo_lost_cnt);
      canErrPrint(i);
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "read"))
  {
    uint32_t index = 0;

    while(cliKeepLoop())
    {
      canUpdate();

      if (canMsgAvailable(_DEF_CAN1))
      {
        can_msg_t msg;

        canMsgRead(_DEF_CAN1, &msg);

        index %= 1000;
        cliPrintf("%03d(R) <- id ", index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "send"))
  {
    uint32_t pre_time;
    uint32_t index = 0;
    uint32_t err_code;


    err_code = can_tbl[_DEF_CAN1].err_code;

    while(cliKeepLoop())
    {
      can_msg_t msg;

      if (millis()-pre_time >= 1000)
      {
        pre_time = millis();

        msg.frame   = CAN_CLASSIC;
        msg.id_type = CAN_EXT;
        msg.dlc     = CAN_DLC_2;
        msg.id      = 0x314;
        msg.length  = 2;
        msg.data[0] = 1;
        msg.data[1] = 2;
        if (canMsgWrite(_DEF_CAN1, &msg, 10) > 0)
        {
          index %= 1000;
          cliPrintf("%03d(T) -> id ", index++);
          if (msg.id_type == CAN_STD)
          {
            cliPrintf("std ");
          }
          else
          {
            cliPrintf("ext ");
          }
          cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
          for (int i=0; i<msg.length; i++)
          {
            cliPrintf("0x%02X ", msg.data[i]);
          }
          cliPrintf("\n");
        }
        else
        {
          cliPrintf("err %d \n", mcp2515ReadErrorFlags(can_tbl[_DEF_CAN1].hfdcan));
        }


        if (canGetRxErrCount(_DEF_CAN1) > 0 || canGetTxErrCount(_DEF_CAN1) > 0)
        {
          cliPrintf("ErrCnt : %d, %d\n", canGetRxErrCount(_DEF_CAN1), canGetTxErrCount(_DEF_CAN1));
        }

        if (err_int_cnt > 0)
        {
          cliPrintf("Cnt : %d\n",err_int_cnt);
          err_int_cnt = 0;
        }
      }

      if (can_tbl[_DEF_CAN1].err_code != err_code)
      {
        cliPrintf("ErrCode : 0x%X\n", can_tbl[_DEF_CAN1].err_code);
        canErrPrint(_DEF_CAN1);
        err_code = can_tbl[_DEF_CAN1].err_code;
      }

      if (canUpdate())
      {
        cliPrintf("BusOff Recovery\n");
      }


      if (canMsgAvailable(_DEF_CAN1))
      {
        canMsgRead(_DEF_CAN1, &msg);

        index %= 1000;
        cliPrintf("%03d(R) <- id ", index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("can info\n");
    cliPrintf("can read\n");
    cliPrintf("can send\n");
  }
}
#endif

#endif
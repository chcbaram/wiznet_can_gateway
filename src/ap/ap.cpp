#include "ap.h"
#include "EthernetManager.hpp"

static cmd_can_t cmd_can;
static cmd_can_driver_t cmd_can_driver;

static qbuffer_t *sendBufferPtr;
static qbuffer_t *recvBufferPtr;

static void apCore1();
static uint32_t canDriverAvailable(void);
static bool canDriverFlush(void);
static uint8_t canDriverRead(void);
static uint32_t canDriverWrite(uint8_t *p_data, uint32_t length);

void apInit(void) {
  qbufferInit();

  cmd_can_driver.available = canDriverAvailable;
  cmd_can_driver.flush = canDriverFlush;
  cmd_can_driver.read = canDriverRead;
  cmd_can_driver.write = canDriverWrite;

  cmdCanInit(&cmd_can, &cmd_can_driver);
  cmdCanOpen(&cmd_can);

  cliOpen(_DEF_UART1, 115200);
  canOpen(_DEF_CAN1, CAN_NORMAL, CAN_CLASSIC, CAN_1M, CAN_1M);

  for (int i = 0; i < 32; i += 1) {
    lcdClearBuffer(black);
    lcdPrintfResize(0, 40 - i, green, 16, "  -- WIZnet --");
    lcdDrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, white);
    lcdUpdateDraw();
    delay(10);
  }
  delay(1000);

  lcdClearBuffer(black);
  lcdPrintfResize(0, 0, green, 16, "Getting IP..");
  lcdUpdateDraw();

  EthernetManager::InitInstance(MAC_ADDRESS, DHCP_ENABLED);
  EthernetManager::GetInstance()->AddSocket(std::make_shared<EventSocket>(SOCKET_PORT_DEFAULT, SocketMode::UDP_PEER));

  multicore_launch_core1(apCore1);
}

void apMain(void) {
  uint32_t pre_time = 0;
  uint32_t heart_cnt = 0;
  uint32_t can_cnt = 0;

  auto socket = EthernetManager::GetInstance()->GetSocket();
  if (socket) {
    sendBufferPtr = socket->GetSendBuffer();
    recvBufferPtr = socket->GetRecvBuffer();
  }

  while (1) {
    if (millis() - pre_time >= 500) {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      lcdClearBuffer(black);
      lcdPrintf(0, 0, white, "IP %d.%d.%d.%d", EthernetManager::GetInstance()->GetIPAddress()[0],
                EthernetManager::GetInstance()->GetIPAddress()[1], EthernetManager::GetInstance()->GetIPAddress()[2],
                EthernetManager::GetInstance()->GetIPAddress()[3]);
      lcdPrintf(0, 16, white, "   %d %4d", SOCKET_PORT_DEFAULT, can_cnt);
      lcdUpdateDraw();

      can_msg_t can_msg;

      can_msg.id = 0x123;
      can_msg.id_type = CAN_EXT;
      can_msg.frame = CAN_CLASSIC;
      can_msg.frame_type = CAN_FRAME_TYPE_DATA;
      can_msg.length = 4;
      memcpy(can_msg.data, &can_cnt, 4);
      can_cnt++;

      cmdCanSendType(&cmd_can, PKT_TYPE_CAN, (uint8_t *)&can_msg, sizeof(can_msg));
    }

// TODO: Ethernet2CAN
// Loopback test code
#if 0
    {
      auto socket = EthernetManager::GetInstance()->GetSocket();
      if (socket) {
        qbuffer_t* sendBufferPtr = socket->GetSendBuffer();
        qbuffer_t* recvBufferPtr = socket->GetRecvBuffer();

        uint32_t recvSize = qbufferAvailable(recvBufferPtr);
        if (recvSize > 0) {
          uint8_t tempBuffer[recvSize];
          qbufferRead(recvBufferPtr, tempBuffer, recvSize);
          cliPrintf("recv %d bytes\r\n", recvSize);

          if (qbufferWrite(sendBufferPtr, tempBuffer, recvSize) == false) {
            cliPrintf("send buffer full\r\n");
          }
        }
      }
    }
#else
    if (cmdCanReceivePacket(&cmd_can) == true) {
      if (cmd_can.rx_packet.type == PKT_TYPE_CMD) {
        logPrintf("Receive Cmd : 0x%02X\n", cmd_can.rx_packet.cmd);
        switch (cmd_can.rx_packet.cmd) {
          case PKT_CMD_PING:
            memcpy(cmd_can.packet.data, &heart_cnt, sizeof(heart_cnt));
            cmdCanSendResp(&cmd_can, cmd_can.rx_packet.cmd, cmd_can.packet.data, sizeof(heart_cnt));
            heart_cnt++;
            break;

          default:
            break;
        }
      }
      if (cmd_can.rx_packet.type == PKT_TYPE_PING) {
        logPrintf("Receive Type Ping\n");
      }
      if (cmd_can.rx_packet.type == PKT_TYPE_CAN) {
        can_msg_t can_msg;

        memcpy(&can_msg, cmd_can.rx_packet.data, sizeof(can_msg));

        logPrintf("rx can msg id:0x%X, ext:%d dlc:%d, ", can_msg.id, can_msg.id_type, can_msg.length);
        for (int i = 0; i < can_msg.length; i++) {
          logPrintf("0x%02X, ", can_msg.data[i]);
        }
        logPrintf("\n");
      }
    }
#endif

    cliMain();
  }
}

static void apCore1() {
  uint32_t pre_time = 0;

  while (1) {
    EthernetManager::GetInstance()->Run();
    if (millis() - pre_time >= 500) {
      cliPrintf("running EthernetManager\r\n");
      pre_time = millis();
    }
    delay(1);
  }
}

uint32_t canDriverAvailable(void) {
  if (recvBufferPtr)
    return qbufferAvailable(recvBufferPtr);
  else
    return 0;
}

bool canDriverFlush(void) {
  if (recvBufferPtr) {
    qbufferFlush(recvBufferPtr);
  }
  return true;
}

uint8_t canDriverRead(void) {
  uint8_t ret;

  if (recvBufferPtr) {
    qbufferRead(recvBufferPtr, &ret, 1);
  }
  return ret;
}

uint32_t canDriverWrite(uint8_t *p_data, uint32_t length) {
  int ret = 0;

  if (sendBufferPtr == nullptr) return 0;

  if (qbufferWrite(sendBufferPtr, p_data, length) == true) {
    ret = length;
  }

  if (ret < 0) ret = 0;

  return ret;
}
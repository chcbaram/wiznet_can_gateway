#include "ap.h"
#include "EthernetManager.hpp"

static void apCore1();

void apInit(void) {
  qbufferInit();

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

  while (1) {
    if (millis() - pre_time >= 500) {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      lcdPrintfResize(0, 0, white, 16, "IP %d.%d.%d.%d, %d", EthernetManager::GetInstance()->GetIPAddress()[0],
                      EthernetManager::GetInstance()->GetIPAddress()[1],
                      EthernetManager::GetInstance()->GetIPAddress()[2],
                      EthernetManager::GetInstance()->GetIPAddress()[3], pre_time);
      lcdUpdateDraw();

      cliPrintf("IP %d.%d.%d.%d, %d\n", EthernetManager::GetInstance()->GetIPAddress()[0],
                EthernetManager::GetInstance()->GetIPAddress()[1], EthernetManager::GetInstance()->GetIPAddress()[2],
                EthernetManager::GetInstance()->GetIPAddress()[3], pre_time);
    }

    // TODO: Ethernet2CAN
    // Loopback test codes
    if (EthernetManager::GetInstance()->IsAssignedIP()) {
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

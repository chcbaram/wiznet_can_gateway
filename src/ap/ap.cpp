#include "ap.h"
#include "EthernetManager.hpp"

static qbuffer_t ethSendRingBuffer, ethRecvRingBuffer;
static uint8_t ethSharedSendBuffer[ETH_SHARED_BUFFER_SIZE], ethSharedRecvBuffer[ETH_SHARED_BUFFER_SIZE];

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
    }

    // TODO: Ethernet2CAN
    // Loopback test code
    {
      uint32_t recvSize = qbufferAvailable(&ethRecvRingBuffer);
      if (recvSize > 0) {
        uint8_t recvBuffer[recvSize];
        qbufferRead(&ethRecvRingBuffer, recvBuffer, recvSize);
        // cliPrintf("recv %d bytes\r\n", recvSize);

        if (qbufferWrite(&ethSendRingBuffer, recvBuffer, recvSize) == false) {
          cliPrintf("send buffer full\r\n");
        }
      }
    }

    cliMain();
  }
}

static void apCore1() {
  uint32_t pre_time = 0;

  qbufferCreate(&ethSendRingBuffer, ethSharedSendBuffer, sizeof(ethSharedSendBuffer));
  qbufferCreate(&ethRecvRingBuffer, ethSharedRecvBuffer, sizeof(ethSharedRecvBuffer));

  EthernetManager::GetInstance()->AddSocket(
      std::make_shared<EventSocket>(SOCKET_PORT_DEFAULT, ethSendRingBuffer, ethRecvRingBuffer, SocketMode::UDP_PEER));

  while (1) {
    EthernetManager::GetInstance()->Run();
    if (millis() - pre_time >= 500) {
      pre_time = millis();
    }
    delay(1);
  }
}

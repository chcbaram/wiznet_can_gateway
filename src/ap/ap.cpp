#include "ap.h"
#include "EthernetManager.hpp"
#include "can_driver.h"

static cmd_can_t cmd_can;
static void apCore1();
static void Ethernet2CAN(void);
static void EthernetLoopBack(void);

static bool is_connected = false;
static bool is_init = false;


void apInit(void) {
  uint8_t cnt_i = 0;

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


  multicore_launch_core1(apCore1);  

  while(is_init != true) {

    ledToggle(_DEF_LED1);

    lcdClearBuffer(black);
    
    if ((cnt_i%8)/2 == 0) lcdPrintfResize(6, 8, green, 16, "Connectting");    
    if ((cnt_i%8)/2 == 1) lcdPrintfResize(6, 8, green, 16, "Connectting.");    
    if ((cnt_i%8)/2 == 2) lcdPrintfResize(6, 8, green, 16, "Connectting..");    
    if ((cnt_i%8)/2 == 3) lcdPrintfResize(6, 8, green, 16, "Connectting...");    
    cnt_i++;
    
    lcdUpdateDraw();
    delay(100);
  }
  lcdClear(black);

  cmdCanInit(&cmd_can, canDriverGetPtr());
  cmdCanOpen(&cmd_can);
}

void apMain(void) {
  uint32_t pre_time = 0;

  while (1) {
    if (millis() - pre_time >= 500) {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      lcdClearBuffer(black);
      if (EthernetManager::GetInstance()->IsAssignedIP()) {
        lcdPrintf(0, 0, white, "IP %d.%d.%d.%d", EthernetManager::GetInstance()->GetIPAddress()[0],
                  EthernetManager::GetInstance()->GetIPAddress()[1], EthernetManager::GetInstance()->GetIPAddress()[2],
                  EthernetManager::GetInstance()->GetIPAddress()[3]);

        lcdPrintf(0, 16, white, "%d  %d DHCP", (millis() / 1000) % 10, SOCKET_PORT_DEFAULT);

        if (is_connected) {
          lcdDrawFillRect(LCD_WIDTH-16, 16, 10, 16, white);
          lcdPrintf(LCD_WIDTH-16, 16, black, "C");
        }
      } else {
        lcdPrintfResize(0, 0, green, 16, "Getting IP..");
      }
      lcdUpdateDraw();
    }

    if (canIsOpen(_DEF_CAN1)) {
      Ethernet2CAN();
    } else {
      EthernetLoopBack();
    }

    cliMain();
  }
}

static void apCore1() {
  uint32_t pre_time = 0;

  EthernetManager::InitInstance(MAC_ADDRESS, DHCP_ENABLED);
  EthernetManager::GetInstance()->AddSocket(std::make_shared<EventSocket>(SOCKET_PORT_DEFAULT, SocketMode::UDP_PEER));

  is_init = true;

  while (1) {
    EthernetManager::GetInstance()->Run();
    if (millis() - pre_time >= 500) {
      AP_LOGGER_PRINT("running EthernetManager\r\n");
      pre_time = millis();
    }
    delay(1);
  }
}

void EthernetLoopBack(void) {
  if (EthernetManager::GetInstance()->IsAssignedIP()) {
    auto socket = EthernetManager::GetInstance()->GetSocket();
    if (socket) {
      qbuffer_t* sendBufferPtr = socket->GetSendBuffer();
      qbuffer_t* recvBufferPtr = socket->GetRecvBuffer();

      uint32_t recvSize = qbufferAvailable(recvBufferPtr);
      if (recvSize > 0) {
        uint8_t tempBuffer[recvSize];
        qbufferRead(recvBufferPtr, tempBuffer, recvSize);
        AP_LOGGER_PRINT("recv %d bytes\r\n", recvSize);

        if (qbufferWrite(sendBufferPtr, tempBuffer, recvSize) == false) {
          AP_LOGGER_PRINT("send buffer full\r\n");
        }
      }
    }
  }
}

enum {
  LED_STATE_IDLE, 
  LED_STATE_ON,
  LED_STATE_OFF
};

void Ethernet2CAN(void) {
  static uint32_t heart_cnt = 0;
  static uint32_t ping_pre_time = millis();
  static uint32_t led_pre_time[2];
  static uint8_t led_cnt[2] = {0, };
  static uint8_t led_state[2] = {0, };

  canUpdate();

  // LED Update
  //
  for (int i=0; i<2; i++) {
    switch(led_state[i]) {
      case LED_STATE_IDLE:
        if (led_cnt[i] > 0) {
          led_pre_time[i] = millis();
          led_state[i] = LED_STATE_ON;
          ledOn(_DEF_LED2 + i);
        }
        break;
      
      case LED_STATE_ON:
        if (millis()-led_pre_time[i] >= 50) {
          led_pre_time[i] = millis();
          led_state[i] = LED_STATE_OFF;
          ledOff(_DEF_LED2 + i);
        }
        break;

      case LED_STATE_OFF:
        if (millis()-led_pre_time[i] >= 50) {
          led_state[i] = LED_STATE_IDLE;
          led_cnt[i] = 0;
        }
        break;
    }
  }

  // Connection Check
  //
  if (millis()-ping_pre_time > 2000) {
    is_connected = false;
  }

  // CAN -> Ethernet
  //
  if (canMsgAvailable(_DEF_CAN1)) {
    can_msg_t can_msg;

    led_cnt[0]++;
    canMsgRead(_DEF_CAN1, &can_msg);
    AP_LOGGER_PRINT("%d %d %d\n", can_msg.dlc, can_msg.length, can_msg.data[1]);
    can_msg.dlc = canGetLenToDlc(can_msg.length);
    cmdCanSendType(&cmd_can, PKT_TYPE_CAN, (uint8_t*)&can_msg, sizeof(can_msg));    
  }

  // Ethernet -> CAN
  //
  if (cmdCanReceivePacket(&cmd_can) == true) {    

    led_cnt[1]++;
    if (cmd_can.rx_packet.type == PKT_TYPE_CMD) {
      AP_LOGGER_PRINT("Receive Cmd : 0x%02X\n", cmd_can.rx_packet.cmd);
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
      AP_LOGGER_PRINT("Receive Type Ping\n");

      ping_pre_time = millis();
      is_connected = true;

      uint8_t status = 0;
      status = canGetError(_DEF_CAN1);
      cmdCanSendType(&cmd_can, PKT_TYPE_PING, &status, 1);
    }
    if (cmd_can.rx_packet.type == PKT_TYPE_CAN) {
      can_msg_t can_msg;

      memcpy(&can_msg, cmd_can.rx_packet.data, sizeof(can_msg));
      can_msg.dlc = canGetLenToDlc(can_msg.length);

      if (canMsgWrite(_DEF_CAN1, &can_msg, 10) != true) {
        AP_LOGGER_PRINT("canMsgWrite Fail\n");
      }

#if 0
      AP_LOGGER_PRINT("rx can msg id:0x%X, ext:%d dlc:%d, ", can_msg.id, can_msg.id_type, can_msg.length);
      for (int i=0; i<can_msg.length; i++)
      {
        AP_LOGGER_PRINT("0x%02X, ", can_msg.data[i]);
      }
      AP_LOGGER_PRINT("\n");
#endif
    }
  }
}


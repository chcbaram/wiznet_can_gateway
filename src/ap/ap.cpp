#include "ap.h"



void apInit(void)
{
  cliOpen(_DEF_UART1, 115200);
  canOpen(_DEF_CAN1, CAN_NORMAL, CAN_CLASSIC, CAN_1M, CAN_1M);

  for (int i=0; i<32; i+=1)
  {
    lcdClearBuffer(black);
    lcdPrintfResize(0, 40-i, green, 16, "  -- WIZnet --");  
    lcdDrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, white);
    lcdUpdateDraw();
    delay(10);
  }
  delay(1000);

  lcdClearBuffer(black);
  lcdPrintfResize(0, 0, green, 16, "Getting IP..");  
  lcdUpdateDraw();

  //multicore_launch_core1(loopback_test_main);
  //loopback_test_main();
  //dhcp_test_main();
  multicore_launch_core1(dhcp_test_main);
}

void apMain(void)
{
  uint32_t pre_time;
  bool pre_dhcp_init = false;

  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (dhcpIsInit() != pre_dhcp_init)
    {
      lcdClearBuffer(black);
      if (dhcpIsInit() == true)
      {
        wiz_NetInfo net_info;

        dhcpGetNetInfo(&net_info);
        lcdPrintfResize(0, 0, white, 16, 
                        "IP %d.%d.%d.%d",
                        net_info.ip[0],
                        net_info.ip[1],
                        net_info.ip[2],
                        net_info.ip[3]
                        );          
      }
      else
      {
        lcdPrintfResize(0, 0, white, 16, "dhcp is empty");
      }

      lcdUpdateDraw();
      pre_dhcp_init = dhcpIsInit();
    }

    cliMain();
  }
}


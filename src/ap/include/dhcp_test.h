#pragma once


#include "ap_def.h"


extern "C"
{
#include "port_common.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "dhcp.h"
#include "dns.h"
#include "timer.h"
}


void dhcp_test_main();

bool dhcpIsInit(void);
bool dhcpGetNetInfo(wiz_NetInfo *p_info);
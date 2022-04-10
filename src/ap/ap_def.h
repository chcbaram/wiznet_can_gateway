/*
 * ap_def.h
 *
 *  Created on: Jun 13, 2021
 *      Author: baram
 */

#ifndef SRC_AP_AP_DEF_H_
#define SRC_AP_AP_DEF_H_

#include <array>
#include "hw.h"

/* Const parameters for Ethernet */
const bool DHCP_ENABLED = true;
const std::array<uint8_t, 6> MAC_ADDRESS = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
const uint16_t SOCKET_PORT_DEFAULT = 4444;
const bool DHCP_LOG = false;

#endif /* SRC_AP_AP_DEF_H_ */

#include "EthernetManager.hpp"
#include <bsp.h>
#include <socket.h>
#include <wizchip_conf.h>

std::shared_ptr<EthernetManager> EthernetManager::instance = nullptr;

std::shared_ptr<EthernetManager> EthernetManager::InitInstance(const uint8_t (&mac)[6], const uint8_t (&ipAddr)[4],
                                                               const uint8_t (&subnet)[4], const uint8_t (&gateway)[4],
                                                               const uint8_t (&dns)[4], bool dhcpEnable) {
  if (instance == nullptr) {
    instance = std::make_shared<EthernetManager>(mac, ipAddr, subnet, gateway, dns, dhcpEnable);
  }
  return instance;
}

std::shared_ptr<EthernetManager> EthernetManager::GetInstance() {
  if (instance != nullptr) {
    return instance;
  }
  return nullptr;
}

EthernetManager::EthernetManager(const uint8_t (&mac)[6], const uint8_t (&ipAddr)[4], const uint8_t (&subnet)[4],
                                 const uint8_t (&gateway)[4], const uint8_t (&dns)[4], bool dhcpEnable)
    : dhcpEnabled_(dhcpEnable),
      DHCP_RETRY_TIMEOUT_MS(5 * 1000),
      preTimeDHCP_(0),
      socketNum_(0),
      socketMode_(SocketMode::UDP_PEER),
      localPort_(0),
      remotePort_(0),
      remoteIpAddr_({}),
      sendBuffer_(nullptr),
      recvBuffer_(nullptr),
      socketOpened_(false) {
  memcpy(MAC_, mac, sizeof(MAC_));
  memcpy(ipAddr_, ipAddr, sizeof(ipAddr_));
  memcpy(subnet_, subnet, sizeof(subnet_));
  memcpy(gateway_, gateway, sizeof(gateway_));
  memcpy(dns_, dns, sizeof(dns_));

  if (dhcpEnabled_) {
    preTimeDHCP_ = millis();
  }
}

bool EthernetManager::Open(uint16_t port, SocketMode mode) {
  if (SocketOpen(port, mode)) {
    localPort_ = port;
    socketMode_ = mode;
    socketOpened_ = true;
    return true;
  }
  return false;
}

void EthernetManager::Run() {
  if (dhcpEnabled_) {
    MaintainDHCP();
  }

  // TODO: check socket status

  if (socketOpened_) {
    OnReadable();
    OnWritable();
  }
}

void EthernetManager::MaintainDHCP() {
  if (millis() - preTimeDHCP_ < DHCP_RETRY_TIMEOUT_MS) {
    // get dhcp status
    // if dhcp status == leased ? preTimeDHCP_ = millis();
  }
}

bool EthernetManager::SocketOpen(const uint16_t port, const SocketMode mode) {
  uint8_t protocol;
  uint8_t flag = SF_IO_NONBLOCK;

  if (mode == SocketMode::UDP_PEER) {
    protocol = Sn_MR_UDP;
  } else if (mode == SocketMode::TCP_SERVER) {
    protocol = Sn_MR_TCP;
  } else {
    return false;
  }

  return (socketNum_ == socket(socketNum_, protocol, port, flag));
}

int EthernetManager::OnWritable() {
  uint32_t len = qbufferAvailable(sendBuffer_);
  if (len <= 0) {
    return len;
  } else {
    // check udp packet max length
  }

  // TODO: delegator to split packet payload
  // if qbuffer has a complete packet payload, run below or return
  uint8_t sock_state;
  getsockopt(socketNum_, SO_STATUS, &sock_state);

  switch (sock_state) {
    // Currently only supports UDP
    case SOCK_UDP: {
      uint8_t* buf = new uint8_t[len];  // TODO: static buffer
      if (buf) {
        if (qbufferRead(sendBuffer_, buf, len)) {
          len = sendto(socketNum_, buf, len, remoteIpAddr_, remotePort_);
        } else {
          len = 0;
        }
        delete[] buf;
      }
    }
    case SOCK_ESTABLISHED:
    default:
      break;
  }
}

int EthernetManager::OnReadable() {
  int len = 0;
  uint8_t sock_state;
  getsockopt(socketNum_, SO_STATUS, &sock_state);

  switch (sock_state) {
    // Currently only supports UDP
    case SOCK_UDP: {
      getsockopt(socketNum_, SO_RECVBUF, &len);

      if (len > 0) {
        uint32_t remain = qbufferAvailable(recvBuffer_);
        if (len > remain) len = remain;
        uint8_t* buf = new uint8_t[len];  // TODO: static buffer
        if (buf) {
          len = recvfrom(socketNum_, buf, len, remoteIpAddr_, &remotePort_);
          if (len) {
            qbufferWrite(recvBuffer_, buf, len);
          }
          delete[] buf;
        }
      }
      break;
    }

    case SOCK_ESTABLISHED:
    default:
      break;
  }

  return len;
}

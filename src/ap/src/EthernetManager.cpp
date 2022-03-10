#include <bsp.h>
#include <dhcp.h>
#include <socket.h>
#include <w5x00_spi.h>
#include <wizchip_conf.h>
#include <EthernetManager.hpp>
#include <cstring>

std::shared_ptr<EthernetManager> EthernetManager::instance = nullptr;

std::shared_ptr<EthernetManager> EthernetManager::InitInstance(const std::array<uint8_t, 6> &mac, bool dhcpEnable) {
  if (instance == nullptr) {
    instance = std::make_shared<EthernetManager>(mac, dhcpEnable);
  }
  return instance;
}

std::shared_ptr<EthernetManager> EthernetManager::GetInstance() { return instance; }

EthernetManager::EthernetManager(const std::array<uint8_t, 6> &mac, bool dhcpEnable)
    : MAC_ADDRESS_(mac), ip_address_({}), dhcpEnabled_(dhcpEnable), DHCP_RETRY_TIMEOUT_MS(5 * 1000), preTimeDHCP_(0) {
  stdio_init_all();

  wizchip_spi_initialize();
  wizchip_cris_initialize();

  wizchip_reset();
  wizchip_initialize();
  wizchip_check();

  if (dhcpEnabled_) {
    // DHCP_init(0, bufferToDHCP);
    // reg_dhcp_cbfunc(ip_assign, ip_update, ip_conflict);
    preTimeDHCP_ = millis();
  } else {
    setSHAR((uint8_t *)MAC_ADDRESS_.data());
    setSIPR((uint8_t *)STATIC_IP_ADDRESS.data());
    setSUBR((uint8_t *)STATIC_SUBNET_MASK.data());
    setGAR((uint8_t *)STATIC_GATEWAY_ADDRESS.data());
    ip_address_ = STATIC_IP_ADDRESS;
  }
}

void EthernetManager::Run() {
  if (dhcpEnabled_) {
    MaintainDHCP();
  }

  // TODO: check socket status

  if (socket_ && socket_->IsOpened()) {
    socket_->OnReadable();
    socket_->OnWritable();
  }
}

void EthernetManager::MaintainDHCP() {
  if (millis() - preTimeDHCP_ < DHCP_RETRY_TIMEOUT_MS) {
    // get dhcp status
    // if dhcp status == leased ? preTimeDHCP_ = millis();
  }
}

bool EthernetManager::AddSocket(std::shared_ptr<EventSocket> socket) {
  if (socket_ != nullptr || socket == nullptr) {
    return false;
  }
  socket_ = socket;
  return true;
}

std::shared_ptr<EventSocket> EthernetManager::GetSocket() { return socket_; }

std::array<uint8_t, 4> EthernetManager::GetIPAddress() const { return ip_address_; }

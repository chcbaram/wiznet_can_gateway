#include <bsp.h>
#include <cli.h>
#include <dhcp.h>
#include <socket.h>
#include <w5x00_spi.h>
#include <wizchip_conf.h>
#include <EthernetManager.hpp>
#include <cstring>

static uint8_t bufferToDHCP[1024] = {
    0,
};

std::shared_ptr<EthernetManager> EthernetManager::instance = nullptr;

std::shared_ptr<EthernetManager> EthernetManager::InitInstance(const std::array<uint8_t, 6> &mac, bool dhcpEnable) {
  if (instance == nullptr) {
    instance = std::make_shared<EthernetManager>(mac, dhcpEnable);
  }
  return instance;
}

std::shared_ptr<EthernetManager> EthernetManager::GetInstance() { return instance; }

EthernetManager::EthernetManager(const std::array<uint8_t, 6> &mac, bool dhcpEnable)
    : MAC_ADDRESS_(mac),
      ip_address_({}),
      isAssignedIP_(false),
      dhcpEnabled_(dhcpEnable),
      preTimeDHCP_(0),
      dhcpRetryCnt_(0) {
  stdio_init_all();

  wizchip_spi_initialize();
  wizchip_cris_initialize();

  wizchip_reset();
  wizchip_initialize();
  wizchip_check();

  if (dhcpEnabled_) {
    // 1s timer
    {
      auto timerCb = [](struct repeating_timer *t) -> bool {
        DHCP_time_handler();
        return true;
      };
      add_repeating_timer_ms(-1000, timerCb, NULL, &timerToDHCP_);
    }

    // Register callbacks
    {
      auto ipAssignCb = [](void) {
        wiz_NetInfo netInfo;
        getIPfromDHCP(netInfo.ip);
        getGWfromDHCP(netInfo.gw);
        getSNfromDHCP(netInfo.sn);
        getDNSfromDHCP(netInfo.dns);
        netInfo.dhcp = NETINFO_DHCP;
        network_initialize(netInfo);  // apply from DHCP
      };
      auto ipUpdateCb = ipAssignCb;
      auto ipConflictCb = [](void) {
        cliPrintf(" Conflict IP from DHCP\n");
        // halt or reset or any...
        while (1)
          ;
      };
      reg_dhcp_cbfunc(ipAssignCb, ipUpdateCb, ipConflictCb);
    }

    DHCP_init(0, bufferToDHCP);
  } else {
    setSHAR((uint8_t *)MAC_ADDRESS_.data());
    setSIPR((uint8_t *)STATIC_IP_ADDRESS.data());
    setSUBR((uint8_t *)STATIC_SUBNET_MASK.data());
    setGAR((uint8_t *)STATIC_GATEWAY_ADDRESS.data());
    ip_address_ = STATIC_IP_ADDRESS;

    isAssignedIP_ = true;
  }
}

bool EthernetManager::IsAssignedIP() const { return isAssignedIP_; };

void EthernetManager::Run() {
  if (dhcpEnabled_) {
    MaintainDHCP();
  }

  if (isAssignedIP_ == false) {
    return;
  }

  // TODO: check socket status

  if (socket_ && socket_->IsOpened()) {
    socket_->OnReadable();
    socket_->OnWritable();
  }
}

void EthernetManager::MaintainDHCP() {
  if (millis() - preTimeDHCP_ >= 1000) {
    uint8_t ret = DHCP_run();
    cliPrintf("DHCP: %d\n", ret);

    switch (ret) {
      case DHCP_RUNNING:
      case DHCP_FAILED:
        dhcpRetryCnt_++;
        if (dhcpRetryCnt_ >= 5) {
          DHCP_stop();
          dhcpEnabled_ = false;
          setSHAR((uint8_t *)MAC_ADDRESS_.data());
          setSIPR((uint8_t *)STATIC_IP_ADDRESS.data());
          setSUBR((uint8_t *)STATIC_SUBNET_MASK.data());
          setGAR((uint8_t *)STATIC_GATEWAY_ADDRESS.data());
          ip_address_ = STATIC_IP_ADDRESS;
          isAssignedIP_ = true;
        }
        break;

      case DHCP_IP_LEASED:
        if (isAssignedIP_ == false) {
          dhcpRetryCnt_ = 0;
          getIPfromDHCP(ip_address_.data());
          isAssignedIP_ = true;
        }
        break;

      case DHCP_IP_ASSIGN:
      case DHCP_IP_CHANGED:
      case DHCP_STOPPED:
      default:
        break;
    };

    preTimeDHCP_ = millis();  // MUST BE updated time after Wiznet DHCP_run()
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

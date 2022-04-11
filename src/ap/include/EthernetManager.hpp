#ifndef ETHERNET_MANAGER_H_
#define ETHERNET_MANAGER_H_

#include <timer/timer.h>
#include <array>
#include <memory>
#include "EventSocket.hpp"

class EthernetManager {
 public:
  static std::shared_ptr<EthernetManager> InitInstance(const std::array<uint8_t, 6> &mac, bool dhcpEnable = true);
  static std::shared_ptr<EthernetManager> GetInstance();

  EthernetManager(const std::array<uint8_t, 6> &mac, bool dhcpEnable);

  ~EthernetManager() = default;

  bool IsAssignedIP() const;
  void Run();
  std::array<uint8_t, 4> GetIPAddress() const;
  // NOTE: It support only single socket creation currently
  bool AddSocket(std::shared_ptr<EventSocket> socket);
  std::shared_ptr<EventSocket> GetSocket();

 private:
  static std::shared_ptr<EthernetManager> instance;

  const std::array<uint8_t, 4> STATIC_IP_ADDRESS = {192, 168, 44, 4};
  const std::array<uint8_t, 4> STATIC_SUBNET_MASK = {255, 255, 255, 0};
  const std::array<uint8_t, 4> STATIC_GATEWAY_ADDRESS = {192, 168, 44, 1};

  const std::array<uint8_t, 6> MAC_ADDRESS_;
  std::array<uint8_t, 4> ip_address_;

  bool isAssignedIP_;

  bool dhcpEnabled_;
  uint32_t preTimeDHCP_;
  uint8_t dhcpRetryCnt_;
  struct repeating_timer timerToDHCP_;
  void MaintainDHCP();

  std::shared_ptr<EventSocket> socket_;
};

#endif /* ETHERNET_MANAGER_H_ */

#include <array>
#include <memory>

#include "EventSocket.hpp"

class EthernetManager {
 public:
  static std::shared_ptr<EthernetManager> InitInstance(const std::array<uint8_t, 6> &mac, bool dhcpEnable = true);
  static std::shared_ptr<EthernetManager> GetInstance();

  EthernetManager(const std::array<uint8_t, 6> &mac, bool dhcpEnable);

  ~EthernetManager() = default;

  void Run();
  std::array<uint8_t, 4> GetIPAddress() const;
  // NOTE: It support only single socket creation currently
  bool AddSocket(std::shared_ptr<EventSocket> socket);

 private:
  static std::shared_ptr<EthernetManager> instance;

  const std::array<uint8_t, 4> STATIC_IP_ADDRESS = {192, 168, 44, 4};
  const std::array<uint8_t, 4> STATIC_SUBNET_MASK = {255, 255, 255, 0};
  const std::array<uint8_t, 4> STATIC_GATEWAY_ADDRESS = {192, 168, 44, 1};

  const std::array<uint8_t, 6> MAC_ADDRESS_;
  std::array<uint8_t, 4> ip_address_;

  bool dhcpEnabled_;
  const uint32_t DHCP_RETRY_TIMEOUT_MS;
  uint32_t preTimeDHCP_;
  void MaintainDHCP();

  std::shared_ptr<EventSocket> socket_;
};

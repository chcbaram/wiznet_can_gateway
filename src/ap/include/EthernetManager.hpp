#include <qbuffer.h>
#include <cstdint>
#include <cstring>
#include <memory>

enum class SocketMode {
  UDP_PEER,
  TCP_SERVER,
};

class EthernetManager {
 public:
  static std::shared_ptr<EthernetManager> InitInstance(const uint8_t (&mac)[6], const uint8_t (&ipAddr)[4],
                                                       const uint8_t (&subnet)[4], const uint8_t (&gateway)[4],
                                                       const uint8_t (&dns)[4], bool dhcpEnable = true);
  static std::shared_ptr<EthernetManager> GetInstance();

  EthernetManager(const uint8_t (&mac)[6], const uint8_t (&ipAddr)[4], const uint8_t (&subnet)[4],
                  const uint8_t (&gateway)[4], const uint8_t (&dns)[4], bool dhcpEnable = true);

  ~EthernetManager() = default;

  // NOTE: It support only single socket creation currently
  bool Open(uint16_t port, SocketMode mode = SocketMode::UDP_PEER);
  void Run();

 private:
  static std::shared_ptr<EthernetManager> instance;

  uint8_t MAC_[6], ipAddr_[4], subnet_[4], gateway_[4], dns_[4];

  bool dhcpEnabled_;
  const uint32_t DHCP_RETRY_TIMEOUT_MS;
  uint32_t preTimeDHCP_;
  void MaintainDHCP();

  /* TODO: Socket Object */
 public:
  bool SetSendBuffer(qbuffer_t& buffer);
  bool SetRecvBuffer(qbuffer_t& buffer);

 private:
  uint8_t socketNum_;
  SocketMode socketMode_;
  uint16_t localPort_;
  uint16_t remotePort_;
  uint8_t remoteIpAddr_[4];
  qbuffer_t* sendBuffer_;
  qbuffer_t* recvBuffer_;
  bool socketOpened_;
  bool SocketOpen(const uint16_t port, const SocketMode mode);
  int OnWritable();
  int OnReadable();
};

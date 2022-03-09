#include <qbuffer.h>
#include <cstdbool>
#include <cstdint>

enum class SocketMode {
  UDP_PEER,
  TCP_SERVER,
};

class EventSocket {
 public:
  EventSocket(uint16_t port, qbuffer_t& sendBuffer, qbuffer_t& recvBuffer, SocketMode mode);
  ~EventSocket();

  bool IsOpened() const;

  void OnReadable();
  void OnWritable();

 private:
  uint8_t fd;
  SocketMode mode_;
  uint16_t port_;
  uint16_t remotePort_;
  uint8_t remoteIpAddr_[4];
  qbuffer_t& sendBuffer_;
  qbuffer_t& recvBuffer_;
  bool isOpened_;
};

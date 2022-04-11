#ifndef EVENT_SOCKET_H_
#define EVENT_SOCKET_H_

#include <qbuffer.h>
#include <cstdbool>
#include <cstdint>

const uint16_t ETH_SHARED_BUFFER_SIZE = 512;

enum class SocketMode {
  UDP_PEER,
  TCP_SERVER,
};

class EventSocket {
 public:
  EventSocket(uint16_t port, SocketMode mode);
  ~EventSocket();

  bool IsOpened() const;

  void OnReadable();
  void OnWritable();

  qbuffer_t* GetSendBuffer();  // or Write() and AvailableWrite()
  qbuffer_t* GetRecvBuffer();  // or Read() and Available()

 private:
  uint8_t fd;
  SocketMode mode_;
  uint16_t port_;
  uint16_t remotePort_;
  uint8_t remoteIpAddr_[4];
  bool isOpened_;
  qbuffer_t sendBuffer_;
  qbuffer_t recvBuffer_;
  uint8_t sharedSendBuffer_[ETH_SHARED_BUFFER_SIZE];
  uint8_t sharedRecvBuffer_[ETH_SHARED_BUFFER_SIZE];
};

#endif /* EVENT_SOCKET_H_ */

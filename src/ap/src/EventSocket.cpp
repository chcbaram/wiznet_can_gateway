#include <bsp.h>
#include <socket.h>
#include <wizchip_conf.h>
#include <EventSocket.hpp>
#include <cli.h>

EventSocket::EventSocket(uint16_t port, SocketMode mode)
    : fd(1),
      mode_(mode),
      port_(port),
      remotePort_(0),
      remoteIpAddr_({}),
      isOpened_(false),
      sendBuffer_({}),
      recvBuffer_({}) {
  qbufferCreate(&sendBuffer_, sharedSendBuffer_, sizeof(sharedSendBuffer_));
  qbufferCreate(&recvBuffer_, sharedRecvBuffer_, sizeof(sharedRecvBuffer_));

  uint8_t protocol;
  uint8_t flag = SF_IO_NONBLOCK;

  if (mode_ == SocketMode::UDP_PEER) {
    protocol = Sn_MR_UDP;
  } else if (mode_ == SocketMode::TCP_SERVER) {
    protocol = Sn_MR_TCP;
  }

  isOpened_ = (fd == socket(fd, protocol, port, flag));

  // TODO: impl. process if failed
}

EventSocket::~EventSocket() {
  if (isOpened_) {
    close(fd);
  }
}

bool EventSocket::IsOpened() const { return isOpened_; }

void EventSocket::OnReadable() {
  uint16_t len = 0;
  uint8_t sock_state;
  getsockopt(fd, SO_STATUS, &sock_state);

  switch (sock_state) {
    // Currently only supports UDP
    case SOCK_UDP: {
      getsockopt(fd, SO_RECVBUF, &len);

      if (len > 0) {
        uint32_t remain = (recvBuffer_.len - 1) - qbufferAvailable(&recvBuffer_);
        if (len > (uint16_t)remain) len = remain;

        uint8_t buf[len];
        len = recvfrom(fd, buf, len, remoteIpAddr_, &remotePort_);
        cliPrintf("recvfrom: %d.%d.%d.%d:%d\n", remoteIpAddr_[0], remoteIpAddr_[1], remoteIpAddr_[2], remoteIpAddr_[3],
                  remotePort_);
        if (len) {
          qbufferWrite(&recvBuffer_, buf, len);
        }
      }
      break;
    }

    case SOCK_ESTABLISHED:
    default:
      break;
  }
}

void EventSocket::OnWritable() {
  uint32_t len = qbufferAvailable(&sendBuffer_);
  if (len == 0) {
    return;
  }

  // TODO: delegator to split packet payload
  // if qbuffer has a complete packet payload, run below or return
  uint8_t sock_state;
  getsockopt(fd, SO_STATUS, &sock_state);

  switch (sock_state) {
    // Currently only supports UDP
    case SOCK_UDP: {
      uint8_t buf[len];
      uint16_t sentLen = 0;
      if (qbufferRead(&sendBuffer_, buf, len)) {
        while (sentLen < len) {
          sentLen += sendto(fd, buf + sentLen, len - sentLen, remoteIpAddr_, remotePort_);
        }
      }
    }
    case SOCK_ESTABLISHED:
    default:
      break;
  }
}

qbuffer_t* EventSocket::GetSendBuffer() { return &sendBuffer_; }

qbuffer_t* EventSocket::GetRecvBuffer() { return &recvBuffer_; }

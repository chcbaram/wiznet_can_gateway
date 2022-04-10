#include "can_driver.h"
#include "EthernetManager.hpp"



static uint32_t canDriverAvailable(void);
static bool     canDriverFlush(void);
static uint8_t  canDriverRead(void);
static uint32_t canDriverWrite(uint8_t *p_data, uint32_t length);


static qbuffer_t* sendBufferPtr;
static qbuffer_t* recvBufferPtr;
static cmd_can_driver_t cmd_can_driver;





cmd_can_driver_t *canDriverGetPtr(void)
{
  auto socket = EthernetManager::GetInstance()->GetSocket();
  if(socket) {
    sendBufferPtr = socket->GetSendBuffer();
    recvBufferPtr = socket->GetRecvBuffer();
  }
      
  cmd_can_driver.available = canDriverAvailable;
  cmd_can_driver.flush = canDriverFlush;
  cmd_can_driver.read = canDriverRead;
  cmd_can_driver.write = canDriverWrite;

  return &cmd_can_driver;
}

uint32_t canDriverAvailable(void)
{
  if (recvBufferPtr)
    return qbufferAvailable(recvBufferPtr);
  else 
    return 0;
}

bool canDriverFlush(void)
{
  if (recvBufferPtr) {
    qbufferFlush(recvBufferPtr);
  }
  return true;
}

uint8_t canDriverRead(void)
{
  uint8_t ret;

  if (recvBufferPtr) {
    qbufferRead(recvBufferPtr, &ret, 1);
  }
  return ret;
}

uint32_t canDriverWrite(uint8_t *p_data, uint32_t length)
{
  int ret = 0;

  if (sendBufferPtr == nullptr) return 0;

  if (EthernetManager::GetInstance()->IsAssignedIP()) {
    if (qbufferWrite(sendBufferPtr, p_data, length) == true) {
      ret = length;
    }
  }
  if (ret < 0) ret = 0;

  return ret;
}
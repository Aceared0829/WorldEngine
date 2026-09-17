
#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Threading/Mutex.h>

#  include <poll.h>

class WIpcChannel;
class WPipeChannel_linux;

#  ifndef _W_DEFINED_POLLFD_POD
#    define _W_DEFINED_POLLFD_POD
W_DEFINE_AS_POD_TYPE(struct pollfd);
#  endif

class W_FOUNDATION_DLL WMessageLoop_linux : public WMessageLoop
{
public:
  WMessageLoop_linux();
  ~WMessageLoop_linux();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter) override;

private:
  friend class WPipeChannel_linux;

  enum class WaitType
  {
    Accept,
    IncomingMessage,
    Connect,
    Send
  };

  void RegisterWait(WPipeChannel_linux* pChannel, WaitType type, int fd);
  void RemovePendingWaits(WPipeChannel_linux* pChannel);

private:
  struct WaitInfo
  {
    W_DECLARE_POD_TYPE();

    WPipeChannel_linux* m_pChannel;
    WaitType m_type;
  };

  // m_waitInfos and m_pollInfos are alway the same size.
  // related information is stored at the same index.
  WHybridArray<WaitInfo, 16> m_waitInfos;
  WHybridArray<struct pollfd, 16> m_pollInfos;
  WMutex m_pollMutex;
  WAtomicInteger32 m_numPendingPollModifications = 0;
  int m_wakeupPipeReadEndFd = -1;
  int m_wakeupPipeWriteEndFd = -1;
};

using WMessageLoop_Platform = WMessageLoop_linux;

#endif

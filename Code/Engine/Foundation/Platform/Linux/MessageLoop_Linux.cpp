#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Linux/MessageLoop_Platform.h>
#  include <Foundation/Platform/Linux/PipeChannel_Platform.h>

#  include <fcntl.h>
#  include <poll.h>
#  include <unistd.h>

WMessageLoop_linux::WMessageLoop_linux()
{
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0)
  {
    WLog::Error("[IPC]Failed to create wakeup pipe for WMessageLoop_linux");
  }
  else
  {
    m_wakeupPipeReadEndFd = fds[0];
    m_wakeupPipeWriteEndFd = fds[1];

    // Setup always present wait info for magic index 0 = wake up
    m_pollInfos.PushBack({m_wakeupPipeReadEndFd, POLLIN, 0});
    m_waitInfos.PushBack({nullptr, WaitType::Accept});
  }
}

WMessageLoop_linux::~WMessageLoop_linux()
{
  StopUpdateThread();
  if (m_wakeupPipeReadEndFd >= 0)
  {
    close(m_wakeupPipeReadEndFd);
    close(m_wakeupPipeWriteEndFd);
  }
}

bool WMessageLoop_linux::WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter)
{
  W_ASSERT_ALWAYS(pFilter == nullptr, "Not implemented");

  while (m_numPendingPollModifications > 0)
  {
    WThreadUtils::YieldTimeSlice();
  }

  WLock lock{m_pollMutex};
  int result = poll(m_pollInfos.GetData(), m_pollInfos.GetCount(), iTimeout);
  if (result > 0)
  {
    // Result at index 0 is special and means there was a WakeUp
    if (m_pollInfos[0].revents != 0)
    {
      WUInt8 wakeupByte;
      auto readResult = read(m_wakeupPipeReadEndFd, &wakeupByte, sizeof(wakeupByte));
      W_IGNORE_UNUSED(readResult);
      W_ASSERT_DEV(readResult == sizeof(wakeupByte), "Wakeup byte not read");
      m_pollInfos[0].revents = 0;
      return true;
    }

    for (WUInt32 i = 1; i < m_waitInfos.GetCount();)
    {
      WaitInfo& waitInfo = m_waitInfos[i];
      struct pollfd& pollInfo = m_pollInfos[i];
      if (pollInfo.revents != 0)
      {
        switch (waitInfo.m_type)
        {
          case WaitType::Accept:
            waitInfo.m_pChannel->AcceptIncomingConnection();
            m_pollInfos.RemoveAtAndSwap(i);
            m_waitInfos.RemoveAtAndSwap(i);
            continue;
          case WaitType::Connect:
            waitInfo.m_pChannel->ProcessConnectSuccessfull();
            m_pollInfos.RemoveAtAndSwap(i);
            m_waitInfos.RemoveAtAndSwap(i);
            continue;
          case WaitType::IncomingMessage:
            waitInfo.m_pChannel->ProcessIncomingPackages();
            break;
          case WaitType::Send:
            waitInfo.m_pChannel->InternalSend();
            m_pollInfos.RemoveAtAndSwap(i);
            m_waitInfos.RemoveAtAndSwap(i);
            continue;
        }
        pollInfo.revents = 0;
      }
      ++i;
    }
    return true;
  }
  if (result < 0)
  {
    WLog::Error("[IPC]WMessageLoop_linux failed to poll events. Error {}", errno);
  }
  return false;
}

void WMessageLoop_linux::RegisterWait(WPipeChannel_linux* pChannel, WaitType type, int fd)
{
  short int waitFlags = 0;
  switch (type)
  {
    case WaitType::Accept:
      waitFlags = POLLIN;
      break;
    case WaitType::Connect:
      waitFlags = POLLOUT;
      break;
    case WaitType::IncomingMessage:
      waitFlags = POLLIN;
      break;
    case WaitType::Send:
      waitFlags = POLLOUT;
      break;
  }

  m_numPendingPollModifications.Increment();
  W_SCOPE_EXIT(m_numPendingPollModifications.Decrement());
  WakeUp();
  {
    WLock lock{m_pollMutex};
    m_waitInfos.PushBack({pChannel, type});
    m_pollInfos.PushBack({fd, waitFlags, 0});
  }
}

void WMessageLoop_linux::RemovePendingWaits(WPipeChannel_linux* pChannel)
{
  m_numPendingPollModifications.Increment();
  W_SCOPE_EXIT(m_numPendingPollModifications.Decrement());
  WakeUp();
  {
    WLock lock{m_pollMutex};
    for (WUInt32 i = 0; i < m_pollInfos.GetCount();)
    {
      if (m_waitInfos[i].m_pChannel == pChannel)
      {
        m_waitInfos.RemoveAtAndSwap(i);
        m_pollInfos.RemoveAtAndSwap(i);
      }
      else
      {
        i++;
      }
    }
  }
}

void WMessageLoop_linux::WakeUp()
{
  WUInt8 wakeupByte = 0;
  int writeResult = write(m_wakeupPipeWriteEndFd, &wakeupByte, sizeof(wakeupByte));
  W_IGNORE_UNUSED(writeResult);
  W_ASSERT_DEV(writeResult == sizeof(wakeupByte), "Wakeup byte not written");
}

#endif

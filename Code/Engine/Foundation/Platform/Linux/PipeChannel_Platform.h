#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/IpcChannel.h>

#  include <sys/stat.h>
#  include <sys/types.h>


class W_FOUNDATION_DLL WPipeChannel_linux : public WIpcChannel
{
public:
  WPipeChannel_linux(WStringView sAddress, Mode::Enum mode);
  ~WPipeChannel_linux();

private:
  friend class WMessageLoop;
  friend class WMessageLoop_linux;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  // These are called from MessageLoop_linux on OS events
  void AcceptIncomingConnection();
  void ProcessIncomingPackages();
  void ProcessConnectSuccessfull();

private:
  WString m_serverSocketPath;
  WString m_clientSocketPath;
  int m_serverSocketFd = -1;
  int m_clientSocketFd = -1;

  WUInt8 m_InputBuffer[4096];
  WUInt64 m_previousSendOffset = 0;
};

using WPipeChannel_Platform = WPipeChannel_linux;

#endif

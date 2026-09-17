#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/IpcChannel.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

struct IOContext
{
  OVERLAPPED Overlapped;  ///< Must be first field in class so we can do a reinterpret cast from *Overlapped to *IOContext.
  WIpcChannel* pChannel; ///< Owner of this IOContext.
};

class W_FOUNDATION_DLL WPipeChannel_win : public WIpcChannel
{
public:
  WPipeChannel_win(WStringView sAddress, Mode::Enum mode);
  ~WPipeChannel_win();

private:
  friend class WMessageLoop;
  friend class WMessageLoop_win;

  bool CreatePipe(WStringView sAddress);

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  bool ProcessConnection();
  bool ProcessIncomingMessages(DWORD uiBytesRead);
  bool ProcessOutgoingMessages(DWORD uiBytesWritten);


protected:
  void OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError);

private:
  struct State
  {
    explicit State(WPipeChannel_win* pChannel);
    ~State();
    IOContext Context;
    WAtomicInteger32 IsPending = false; ///< Whether an async operation is in process.
  };

  enum Constants
  {
    BUFFER_SIZE = 4096,
  };

  // Shared data
  State m_InputState;
  State m_OutputState;

  // Setup in ctor
  HANDLE m_hPipeHandle = INVALID_HANDLE_VALUE;

  // Only accessed from worker thread
  WUInt8 m_InputBuffer[BUFFER_SIZE];
};

using WPipeChannel_Platform = WPipeChannel_win;

#endif

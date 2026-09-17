#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/MessageLoop_Platform.h>
#  include <Foundation/Platform/Win/PipeChannel_Platform.h>
#  include <Foundation/Serialization/ReflectionSerializer.h>

WPipeChannel_win::State::State(WPipeChannel_win* pChannel)
  : IsPending(false)
{
  memset(&Context.Overlapped, 0, sizeof(Context.Overlapped));
  Context.pChannel = pChannel;
  IsPending = false;
}

WPipeChannel_win::State::~State() = default;

WPipeChannel_win::WPipeChannel_win(WStringView sAddress, Mode::Enum mode)
  : WIpcChannel(sAddress, mode)
  , m_InputState(this)
  , m_OutputState(this)
{
  m_pOwner->AddChannel(this);
}

WPipeChannel_win::~WPipeChannel_win()
{
  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    Disconnect();
  }
  while (IsConnected())
  {
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }
  m_pOwner->RemoveChannel(this);
}

bool WPipeChannel_win::CreatePipe(WStringView sAddress)
{
  WStringBuilder sPipename("\\\\.\\pipe\\", sAddress);

  if (m_Mode == Mode::Server)
  {
    SECURITY_ATTRIBUTES attributes = {0};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = NULL;
    attributes.bInheritHandle = FALSE;

    m_hPipeHandle = CreateNamedPipeW(WStringWChar(sPipename).GetData(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1, BUFFER_SIZE, BUFFER_SIZE, 5000, &attributes);
  }
  else
  {
    m_hPipeHandle = CreateFileW(WStringWChar(sPipename).GetData(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
      SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION | FILE_FLAG_OVERLAPPED, NULL);
  }

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
  {
    WLog::Error("Could not create named pipe: {0}", WArgErrorCode(GetLastError()));
    return false;
  }

  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    WMessageLoop_win* pMsgLoopWin = static_cast<WMessageLoop_win*>(m_pOwner);

    ULONG_PTR key = reinterpret_cast<ULONG_PTR>(this);
    HANDLE port = CreateIoCompletionPort(m_hPipeHandle, pMsgLoopWin->GetPort(), key, 1);
    W_IGNORE_UNUSED(port);
    W_ASSERT_DEBUG(pMsgLoopWin->GetPort() == port, "Failed to CreateIoCompletionPort: {0}", WArgErrorCode(GetLastError()));
  }
  return true;
}

void WPipeChannel_win::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Connecting)
    return;

#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (m_ThreadId == 0)
    m_ThreadId = WThreadUtils::GetCurrentThreadID();
#  endif

  if (!CreatePipe(m_sAddress))
  {
    SetConnectionState(ConnectionState::Disconnected);
    return;
  }

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
  {
    SetConnectionState(ConnectionState::Disconnected);
    return;
  }

  if (m_Mode == Mode::Server)
  {
    if (!ProcessConnection())
    {
      InternalDisconnect();
      return;
    }
  }
  else
  {
    // If CreatePipe succeeded, we are already connected.
    SetConnectionState(ConnectionState::Connected);
  }

  if (!m_InputState.IsPending)
  {
    OnIOCompleted(&m_InputState.Context, 0, 0);
  }

  if (IsConnected())
  {
    ProcessOutgoingMessages(0);
  }

  return;
}

void WPipeChannel_win::InternalDisconnect()
{
  if (GetConnectionState() == ConnectionState::Disconnected)
    return;

#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (m_ThreadId != 0)
    W_ASSERT_DEBUG(m_ThreadId == WThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
#  endif
  if (m_InputState.IsPending || m_OutputState.IsPending)
  {
    CancelIo(m_hPipeHandle);
  }

  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hPipeHandle);
    m_hPipeHandle = INVALID_HANDLE_VALUE;
  }

  while (m_InputState.IsPending || m_OutputState.IsPending)
  {
    FlushPendingOperations();
  }

  const bool bNeedsDisconnectedEvent = GetConnectionState() != ConnectionState::Disconnected;
  {
    W_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
  }

  if (bNeedsDisconnectedEvent)
  {
    SetConnectionState(ConnectionState::Disconnected);
    // Raise in case another thread is waiting for new messages (as we would sleep forever otherwise).
    m_IncomingMessages.RaiseSignal();
  }
}

void WPipeChannel_win::InternalSend()
{
  if (!m_OutputState.IsPending && IsConnected())
  {
    ProcessOutgoingMessages(0);
  }
}


bool WPipeChannel_win::NeedWakeup() const
{
  return m_OutputState.IsPending == 0;
}

bool WPipeChannel_win::ProcessConnection()
{
  W_ASSERT_DEBUG(m_ThreadId == WThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
    m_InputState.IsPending = false;

  BOOL res = ConnectNamedPipe(m_hPipeHandle, &m_InputState.Context.Overlapped);
  if (res)
  {
    // W_REPORT_FAILURE
    return false;
  }

  WUInt32 error = GetLastError();
  switch (error)
  {
    case ERROR_IO_PENDING:
      m_InputState.IsPending = true;
      break;
    case ERROR_PIPE_CONNECTED:
      SetConnectionState(ConnectionState::Connected);
      break;
    case ERROR_NO_DATA:
      return false;
    default:
      WLog::Error("Could not connect to pipe (Error code: {0})", WArgErrorCode(error));
      return false;
  }

  return true;
}

bool WPipeChannel_win::ProcessIncomingMessages(DWORD uiBytesRead)
{
  W_ASSERT_DEBUG(m_ThreadId == WThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
  {
    m_InputState.IsPending = false;
    if (uiBytesRead == 0)
      return false;
  }

  while (true)
  {
    if (uiBytesRead == 0)
    {
      if (m_hPipeHandle == INVALID_HANDLE_VALUE)
        return false;

      BOOL res = ReadFile(m_hPipeHandle, m_InputBuffer, BUFFER_SIZE, &uiBytesRead, &m_InputState.Context.Overlapped);

      if (!res)
      {
        WUInt32 error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
          m_InputState.IsPending = true;
          return true;
        }
        if (m_Mode == Mode::Server)
        {
          // only log when in server mode, otherwise this can result in an endless recursion
          WLog::Error("Read from pipe failed: {0}", WArgErrorCode(error));
        }
        return false;
      }
      m_InputState.IsPending = true;
      return true;
    }

    W_ASSERT_DEBUG(uiBytesRead != 0, "We really should have data at this point.");
    ReceiveData(WArrayPtr<WUInt8>(m_InputBuffer, uiBytesRead));
    uiBytesRead = 0;
  }
  return true;
}

bool WPipeChannel_win::ProcessOutgoingMessages(DWORD uiBytesWritten)
{
  W_ASSERT_DEBUG(IsConnected(), "Must be connected to process outgoing messages.");
  W_ASSERT_DEBUG(m_ThreadId == WThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");

  if (m_OutputState.IsPending)
  {
    if (uiBytesWritten == 0)
    {
      // Don't reset isPending right away as we want to use it to decide
      // whether we need to wake up the worker thread again.
      WLog::Error("pipe error: {0}", WArgErrorCode(GetLastError()));
      m_OutputState.IsPending = false;
      return false;
    }

    W_LOCK(m_OutputQueueMutex);
    // message was send
    m_OutputQueue.PopFront();
  }

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
  {
    m_OutputState.IsPending = false;
    return false;
  }
  const WMemoryStreamStorageInterface* storage = nullptr;
  {
    W_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      m_OutputState.IsPending = false;
      return true;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  WUInt64 uiToWrite = storage->GetStorageSize64();
  WUInt64 uiNextOffset = 0;
  while (uiToWrite > 0)
  {
    const WArrayPtr<const WUInt8> range = storage->GetContiguousMemoryRange(uiNextOffset);
    uiToWrite -= range.GetCount();

    BOOL res = WriteFile(m_hPipeHandle, range.GetPtr(), range.GetCount(), &uiBytesWritten, &m_OutputState.Context.Overlapped);

    if (!res)
    {
      WUInt32 error = GetLastError();
      if (error == ERROR_IO_PENDING)
      {
        m_OutputState.IsPending = true;
        return true;
      }
      WLog::Error("Write to pipe failed: {0}", WArgErrorCode(error));
      return false;
    }

    uiNextOffset += range.GetCount();
  }


  m_OutputState.IsPending = true;
  return true;
}

void WPipeChannel_win::OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError)
{
  W_IGNORE_UNUSED(uiError);

  W_ASSERT_DEBUG(m_ThreadId == WThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  bool bRes = true;
  if (pContext == &m_InputState.Context)
  {
    if (!IsConnected())
    {
      if (!ProcessConnection())
      {
        InternalDisconnect();
        return;
      }

      bool bHasOutput = false;
      {
        W_LOCK(m_OutputQueueMutex);
        bHasOutput = !m_OutputQueue.IsEmpty();
      }

      if (bHasOutput && m_OutputState.IsPending == 0)
        ProcessOutgoingMessages(0);
      if (m_InputState.IsPending)
        return;
    }
    bRes = ProcessIncomingMessages(uiBytesTransfered);
  }
  else
  {
    W_ASSERT_DEBUG(pContext == &m_OutputState.Context, "");
    bRes = ProcessOutgoingMessages(uiBytesTransfered);
  }
  if (!bRes && m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    InternalDisconnect();
  }
}
#endif

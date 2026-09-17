
#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Linux/MessageLoop_Platform.h>
#  include <Foundation/Platform/Linux/PipeChannel_Platform.h>

#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/un.h>

WPipeChannel_linux::WPipeChannel_linux(WStringView sAddress, Mode::Enum Mode)
  : WIpcChannel(sAddress, Mode)
{
  WStringBuilder pipePath = WOSFile::GetTempDataFolder("W-pipes");

  // Make sure the directory exists that we want to place the pipes in.
  WOSFile::CreateDirectoryStructure(pipePath).IgnoreResult();

  pipePath.AppendPath(sAddress);
  pipePath.Append(".server");

  m_serverSocketPath = pipePath;
  pipePath.Shrink(0, 7); // strip .server
  pipePath.Append(".client");
  m_clientSocketPath = pipePath;

  m_pOwner->AddChannel(this);
}

WPipeChannel_linux::~WPipeChannel_linux()
{
  if (m_pOwner)
  {
    static_cast<WMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);
  }

  if (m_serverSocketFd >= 0)
  {
    close(m_serverSocketFd);
    m_serverSocketFd = -1;
  }

  if (m_clientSocketFd >= 0)
  {
    close(m_clientSocketFd);
    m_clientSocketFd = -1;
  }

  if (m_Mode == Mode::Server)
  {
    WOSFile::DeleteFile(m_serverSocketPath).IgnoreResult();
  }
  else
  {
    WOSFile::DeleteFile(m_clientSocketPath).IgnoreResult();
  }
}

void WPipeChannel_linux::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Connecting)
    return;

  int& targetSocket = (m_Mode == Mode::Server) ? m_serverSocketFd : m_clientSocketFd;

  if (targetSocket < 0)
  {
    const char* thisSocketPath = (m_Mode == Mode::Server) ? m_serverSocketPath.GetData() : m_clientSocketPath.GetData();

    targetSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (targetSocket == -1)
    {
      WLog::Error("[IPC]Failed to create unix domain socket. error {}", errno);
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }

    // If the socket file already exists, delete it
    WOSFile::DeleteFile(thisSocketPath).IgnoreResult();

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;

    if (strlen(thisSocketPath) >= W_ARRAY_SIZE(addr.sun_path) - 1)
    {
      WLog::Error("[IPC]Given ipc channel address is to long. Resulting path '{}' path length limit {}", strlen(thisSocketPath), W_ARRAY_SIZE(addr.sun_path) - 1);
      close(targetSocket);
      targetSocket = -1;
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }

    strcpy(addr.sun_path, thisSocketPath);
    if (bind(targetSocket, (struct sockaddr*)&addr, SUN_LEN(&addr)) == -1)
    {
      WLog::Error("[IPC]Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
      close(targetSocket);
      targetSocket = -1;
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
  }

  if (m_Mode == Mode::Server)
  {
    if (m_serverSocketFd < 0)
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
    if (listen(m_serverSocketFd, 1) != 0)
    {
      WLog::Error("[IPC]Failed to listen on unix domain socket. Error {}", errno);
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
    static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    if (m_clientSocketFd < 0)
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
    struct sockaddr_un serverAddress = {};
    serverAddress.sun_family = AF_UNIX;
    strcpy(serverAddress.sun_path, m_serverSocketPath.GetData());

    connect(m_clientSocketFd, (struct sockaddr*)&serverAddress, SUN_LEN(&serverAddress));

    static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::Connect, m_clientSocketFd);
  }
}

void WPipeChannel_linux::InternalDisconnect()
{
  if (GetConnectionState() == ConnectionState::Disconnected)
    return;

  static_cast<WMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);

  close(m_clientSocketFd);
  m_clientSocketFd = -1;

  {
    W_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
  }

  SetConnectionState(ConnectionState::Disconnected);

  m_IncomingMessages.RaiseSignal(); // Wakeup anyone still waiting for messages
}

void WPipeChannel_linux::InternalSend()
{
  const WMemoryStreamStorageInterface* storage = nullptr;
  {
    W_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      return;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  while (true)
  {

    WUInt64 uiToWrite = storage->GetStorageSize64() - m_previousSendOffset;
    WUInt64 uiNextOffset = m_previousSendOffset;
    while (uiToWrite > 0)
    {
      const WArrayPtr<const WUInt8> range = storage->GetContiguousMemoryRange(uiNextOffset);

      int res = send(m_clientSocketFd, range.GetPtr(), range.GetCount(), 0);

      if (res < 0)
      {
        int errorCode = errno;
        // We can't send at the moment. Wait until we can send again.
        if (errorCode == EWOULDBLOCK)
        {
          m_previousSendOffset = uiNextOffset;
          static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::Send, m_clientSocketFd);
          return;
        }
        WLog::Error("[IPC]WPipeChannel_linux failed to send. Error {}", errorCode);
        InternalDisconnect();
        return;
      }

      uiToWrite -= static_cast<WUInt64>(res);
      uiNextOffset += res;
    }
    m_previousSendOffset = 0;

    {
      W_LOCK(m_OutputQueueMutex);
      m_OutputQueue.PopFront();
      if (m_OutputQueue.IsEmpty())
      {
        return;
      }
      storage = &m_OutputQueue.PeekFront();
    }
  }
}

void WPipeChannel_linux::AcceptIncomingConnection()
{
  struct sockaddr_un incomingConnection = {};
  socklen_t len = sizeof(incomingConnection);
  m_clientSocketFd = accept4(m_serverSocketFd, (struct sockaddr*)&incomingConnection, &len, SOCK_NONBLOCK);
  if (m_clientSocketFd == -1)
  {
    WLog::Error("[IPC]Failed to accept incoming connection. Error {}", errno);
    // Wait for the next incoming connection
    listen(m_serverSocketFd, 1);
    static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    SetConnectionState(ConnectionState::Connected);
    // We are connected. Register for incoming messages events.
    static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
  }
}

bool WPipeChannel_linux::NeedWakeup() const
{
  return true;
}

void WPipeChannel_linux::ProcessConnectSuccessfull()
{
  int errorCode = 0;
  socklen_t errorCodeSize = sizeof(errorCode);
  if (getsockopt(m_clientSocketFd, SOL_SOCKET, SO_ERROR, &errorCode, &errorCodeSize) != 0 || errorCode != 0)
  {
    WLog::Error("[IPC]Failed to connect unix domain socket. Error {}", errorCode != 0 ? errorCode : errno);
    InternalDisconnect();
    return;
  }

  SetConnectionState(ConnectionState::Connected);

  // We are connected. Register for incoming messages events.
  static_cast<WMessageLoop_linux*>(m_pOwner)->RegisterWait(this, WMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
}

void WPipeChannel_linux::ProcessIncomingPackages()
{
  while (true)
  {
    ssize_t recieveResult = recv(m_clientSocketFd, m_InputBuffer, W_ARRAY_SIZE(m_InputBuffer), 0);
    if (recieveResult == 0)
    {
      InternalDisconnect();
      return;
    }

    if (recieveResult < 0)
    {
      int errorCode = errno;
      if (errorCode == EWOULDBLOCK)
      {
        return;
      }
      if (errorCode != ECONNRESET)
      {
        WLog::Error("[IPC]WPipeChannel_linux recieve error {}", errorCode);
      }
      InternalDisconnect();
      return;
    }

    ReceiveData(WArrayPtr(m_InputBuffer, static_cast<WUInt32>(recieveResult)));
  }
}

#endif

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>

WResult WEditorProcessCommunicationChannel::StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const WRTTI* pFirstAllowedMessageType, WUInt32 uiMemSize)
{
  W_LOG_BLOCK("WProcessCommunicationChannel::StartClientProcess");

  W_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  W_ASSERT_DEV(m_pClientProcessGroup == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static WUInt64 uiUniqueHash = 0;
  WOsProcessID PID = WProcess::GetCurrentProcessID();
  uiUniqueHash = WHashingUtils::xxHash64(&PID, sizeof(PID), uiUniqueHash);
  WTime time = WTime::Now();
  uiUniqueHash = WHashingUtils::xxHash64(&time, sizeof(time), uiUniqueHash);
  WStringBuilder sMemName;
  sMemName.SetFormat("{0}", WArgU(uiUniqueHash, 16, true, 16, true));
  ++uiUniqueHash;

  WResult res = W_SUCCESS;
  if (bRemote)
  {
    res = CreateAndConnectChannel(WIpcChannel::CreateNetworkChannel("172.16.80.3:1050", WIpcChannel::Mode::Client));
  }
  else
  {
    res = CreateAndConnectChannel(WIpcChannel::CreatePipeChannel(sMemName, WIpcChannel::Mode::Server));
  }
  if (res.Failed())
  {
    WLog::Error("IpcChannel: CreateAndConnectChannel failed");
    CloseConnection();
    return W_FAILURE;
  }

  for (WUInt32 i = 0; i < 100; i++)
  {
    if (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Connecting)
      break;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }
  if (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connecting)
  {
    WLog::Error("Failed to start IPC server");
    CloseConnection();
    return W_FAILURE;
  }

  WStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = WOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  if (!bRemote)
  {
    WProcessOptions po;
    po.m_sProcess = sPath;
    po.AddArgument("-IPC");
    po.AddArgument(sMemName);
    po.AddArgument("-PID");
    po.AddArgument("{}", WArgU(WProcess::GetCurrentProcessID(), 1, false));
    for (const QString& arg : args)
      po.AddArgument(WStringView(arg.toUtf8().constData()));

    m_pClientProcessGroup = W_DEFAULT_NEW(WProcessGroup);
    if (m_pClientProcessGroup->Launch(po).Failed())
    {
      CloseConnection();
      WLog::Error("Failed to start process '{0}'", sPath);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

bool WEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcessGroup == nullptr)
    return false;
  const auto& processes = m_pClientProcessGroup->GetProcesses();
  if (processes.IsEmpty())
    return false;
  return processes[0].GetState() == WProcessState::Running;
}

void WEditorProcessCommunicationChannel::CloseConnection()
{
  DestroyChannel();
  m_pClientProcessGroup = nullptr;
}

WString WEditorProcessCommunicationChannel::GetStdoutContents()
{
  return WString();
}

WOsProcessID WEditorProcessCommunicationChannel::GetProcessId() const
{
  if (m_pClientProcessGroup == nullptr)
    return {};
  const auto& processes = m_pClientProcessGroup->GetProcesses();
  if (!processes.IsEmpty() && processes[0].GetState() == WProcessState::Running)
    return processes[0].GetProcessID();
  return {};
}

//////////////////////////////////////////////////////////////////////////

WResult WEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  W_LOG_BLOCK("WEditorProcessRemoteCommunicationChannel::ConnectToServer");
  W_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  m_pFirstAllowedMessageType = nullptr;
  if (CreateAndConnectChannel(WIpcChannel::CreateNetworkChannel(szAddress, WIpcChannel::Mode::Client)).Failed())
  {
    WLog::Error("IpcChannel: CreateAndConnectChannel failed");
    CloseConnection();
    return W_FAILURE;
  }

  for (WUInt32 i = 0; i < 200; i++)
  {
    if (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connecting)
      break;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }

  if (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connected)
  {
    WLog::Error("Failed to connect to IPC server");
    CloseConnection();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

bool WEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void WEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  DestroyChannel();
}

void WEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected)
  {
    m_pChannel->Connect().IgnoreResult();
  }
}

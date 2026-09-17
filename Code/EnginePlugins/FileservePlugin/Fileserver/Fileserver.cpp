#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>

W_IMPLEMENT_SINGLETON(WFileserver);

WFileserver::WFileserver()
  : m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  WFileserveClient::DisabledFileserveClient();

  // check whether the fileserve port was reconfigured through the command line
  m_uiPort = static_cast<WUInt16>(WCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_port", m_uiPort));
}

void WFileserver::StartServer()
{
  if (m_pNetwork)
    return;

  WStringBuilder tmp;

  m_pNetwork = WRemoteInterfaceEnet::Make();
  m_pNetwork->StartServer('EZFS', WConversionUtils::ToString(m_uiPort, tmp), false).IgnoreResult();
  m_pNetwork->SetMessageHandler('FSRV', WMakeDelegate(&WFileserver::NetworkMsgHandler, this));
  m_pNetwork->SetUnhandledMessageHandler(WMakeDelegate(&WFileserver::UnknownNetworkMsgHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(WMakeDelegate(&WFileserver::NetworkEventHandler, this));

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void WFileserver::StopServer()
{
  if (!m_pNetwork)
    return;

  m_pNetwork->ShutdownConnection();
  m_pNetwork.Clear();

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool WFileserver::UpdateServer()
{
  if (!m_pNetwork)
    return false;

  m_pNetwork->UpdateRemoteInterface();
  return m_pNetwork->ExecuteAllMessageHandlers() > 0;
}

bool WFileserver::IsServerRunning() const
{
  return m_pNetwork != nullptr;
}

void WFileserver::SetPort(WUInt16 uiPort)
{
  W_ASSERT_DEV(m_pNetwork == nullptr, "The port cannot be changed after the server was started");
  m_uiPort = uiPort;
}


void WFileserver::BroadcastReloadResourcesCommand()
{
  if (!IsServerRunning())
    return;

  m_pNetwork->Send('FSRV', 'RLDR');
}

void WFileserver::NetworkMsgHandler(WRemoteMessage& msg)
{
  auto& client = DetermineClient(msg);

  if (msg.GetMessageID() == 'HELO')
    return;

  if (msg.GetMessageID() == 'RUTR')
  {
    // 'are you there' is used to check whether a certain address is a proper Fileserver
    m_pNetwork->Send('FSRV', ' YES');

    WFileserverEvent e;
    e.m_Type = WFileserverEvent::Type::AreYouThereRequest;
    m_Events.Broadcast(e);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLH')
  {
    HandleUploadFileHeader(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLD')
  {
    HandleUploadFileTransfer(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLF')
  {
    HandleUploadFileFinished(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'DELF')
  {
    HandleDeleteFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == ' MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UMNT')
  {
    HandleUnmountRequest(client, msg);
    return;
  }

  WLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

void WFileserver::UnknownNetworkMsgHandler(WRemoteMessage& msg)
{
  auto it = m_CustomMessageHandlers.Find(msg.GetSystemID());
  if (!it.IsValid() || !it.Value().IsValid())
    return;

  auto& client = DetermineClient(msg);

  it.Value()(client, msg, *m_pNetwork, WMakeDelegate(&WFileserver::LogCustomActivity, this));
}

void WFileserver::NetworkEventHandler(const WRemoteEvent& e)
{
  switch (e.m_Type)
  {
    case WRemoteEvent::DisconnectedFromClient:
    {
      if (m_Clients.Contains(e.m_uiOtherAppID))
      {
        WFileserverEvent se;
        se.m_Type = WFileserverEvent::Type::ClientDisconnected;
        se.m_uiClientID = e.m_uiOtherAppID;

        m_Events.Broadcast(se);

        m_Clients[e.m_uiOtherAppID].m_bLostConnection = true;
      }
    }
    break;

    default:
      break;
  }
}

WFileserveClientContext& WFileserver::DetermineClient(WRemoteMessage& msg)
{
  WFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    WFileserverEvent e;
    e.m_Type = WFileserverEvent::Type::ClientConnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }
  else if (client.m_bLostConnection)
  {
    client.m_bLostConnection = false;

    WFileserverEvent e;
    e.m_Type = WFileserverEvent::Type::ClientReconnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }

  return client;
}

void WFileserver::HandleMountRequest(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  WUInt16 uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  W_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(WMath::Max<WUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;

  WFileserverEvent e;

  if (WFileSystem::ResolveSpecialDirectory(sDataDir, sRedir).Succeeded())
  {
    dir.m_bMounted = true;
    dir.m_sPathOnServer = sRedir;
    e.m_Type = WFileserverEvent::Type::MountDataDir;
  }
  else
  {
    dir.m_bMounted = false;
    e.m_Type = WFileserverEvent::Type::MountDataDirFailed;
  }

  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szName = sRootName;
  e.m_szPath = sDataDir;
  e.m_szRedirectedPath = sRedir;
  m_Events.Broadcast(e);
}


void WFileserver::HandleUnmountRequest(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  W_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::UnmountDataDir;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = dir.m_sPathOnClient;
  e.m_szName = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void WFileserver::HandleFileRequest(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUInt16 uiDataDirID = 0;
  bool bForceThisDataDir = false;

  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> bForceThisDataDir;

  WStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  WUuid downloadGuid;
  msg.GetReader() >> downloadGuid;

  WFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  WFileserverEvent e;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sRequestedFile;
  e.m_uiSentTotal = 0;

  const WFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_SendToClient, bForceThisDataDir);

  {
    e.m_Type = WFileserverEvent::Type::FileDownloadRequest;
    e.m_uiSizeTotal = m_SendToClient.GetCount();
    e.m_FileState = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == WFileserveFileState::Different)
  {
    WUInt32 uiNextByte = 0;
    const WUInt32 uiFileSize = m_SendToClient.GetCount();

    // send the file over in multiple packages of 1KB each
    // send at least one package, even for empty files
    do
    {
      const WUInt16 uiChunkSize = (WUInt16)WMath::Min<WUInt32>(1024, m_SendToClient.GetCount() - uiNextByte);

      WRemoteMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_SendToClient.IsEmpty())
        ret.GetWriter().WriteBytes(&m_SendToClient[uiNextByte], uiChunkSize).IgnoreResult();

      ret.SetMessageID('FSRV', 'DWNL');
      m_pNetwork->Send(WRemoteTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type = WFileserverEvent::Type::FileDownloading;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    } while (uiNextByte < m_SendToClient.GetCount());
  }

  // final answer to client
  {
    WRemoteMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (WInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;
    ret.GetWriter() << uiDataDirID;

    m_pNetwork->Send(WRemoteTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = WFileserverEvent::Type::FileDownloadFinished;
    m_Events.Broadcast(e);
  }
}

void WFileserver::HandleDeleteFileRequest(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  WStringBuilder sFile;
  msg.GetReader() >> sFile;

  W_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::FileDeleteRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  m_Events.Broadcast(e);

  const auto& dd = client.m_MountedDataDirs[uiDataDirID];

  WStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(sFile);

  WOSFile::DeleteFile(sAbsPath).IgnoreResult();
}

void WFileserver::HandleUploadFileHeader(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUInt16 uiDataDirID = 0;

  msg.GetReader() >> m_FileUploadGuid;
  msg.GetReader() >> m_uiFileUploadSize;
  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> m_sCurFileUpload;

  m_SentFromClient.Clear();
  m_SentFromClient.Reserve(m_uiFileUploadSize);

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::FileUploadRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = 0;
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void WFileserver::HandleUploadFileTransfer(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  WUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  const WUInt32 uiStartPos = m_SentFromClient.GetCount();
  m_SentFromClient.SetCountUninitialized(uiStartPos + uiChunkSize);
  msg.GetReader().ReadBytes(&m_SentFromClient[uiStartPos], uiChunkSize);

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::FileUploading;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void WFileserver::HandleUploadFileFinished(WFileserveClientContext& client, WRemoteMessage& msg)
{
  WUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  WUInt16 uiDataDirID = 0;
  msg.GetReader() >> uiDataDirID;

  WStringBuilder sFile;
  msg.GetReader() >> sFile;

  WStringBuilder sOutputFile;
  sOutputFile = client.m_MountedDataDirs[uiDataDirID].m_sPathOnServer;
  sOutputFile.AppendPath(sFile);

  {
    WOSFile file;
    if (file.Open(sOutputFile, WFileOpenMode::Write).Failed())
    {
      WLog::Error("Could not write uploaded file to '{0}'", sOutputFile);
      return;
    }

    if (!m_SentFromClient.IsEmpty())
    {
      file.Write(m_SentFromClient.GetData(), m_SentFromClient.GetCount()).IgnoreResult();
    }
  }

  WFileserverEvent e;
  e.m_Type = WFileserverEvent::Type::FileUploadFinished;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_SentFromClient.GetCount();

  m_Events.Broadcast(e);

  // send a response when all data has been transmitted
  // this ensures the client side updates the network until all data has been fully transmitted
  m_pNetwork->Send('FSRV', 'UACK');
}

void WFileserver::LogCustomActivity(const char* szText)
{
  WFileserverEvent e;
  e.m_szName = szText;
  e.m_Type = WFileserverEvent::Type::LogCustomActivity;
  m_Events.Broadcast(e);
}

WResult WFileserver::SendConnectionInfo(const char* szClientAddress, WUInt16 uiMyPort, const WArrayPtr<WStringBuilder>& myIPs, WTime timeout)
{
  WStringBuilder sAddress = szClientAddress;
  sAddress.Append(":2042"); // hard-coded port

  WUniquePtr<WRemoteInterfaceEnet> network = WRemoteInterfaceEnet::Make();
  W_SUCCEED_OR_RETURN(network->ConnectToServer('EZIP', sAddress, false));

  if (network->WaitForConnectionToServer(timeout).Failed())
  {
    network->ShutdownConnection();
    return W_FAILURE;
  }

  const WUInt8 uiCount = static_cast<WUInt8>(myIPs.GetCount());

  WRemoteMessage msg('FSRV', 'MYIP');
  msg.GetWriter() << uiMyPort;
  msg.GetWriter() << uiCount;

  for (const auto& info : myIPs)
  {
    msg.GetWriter() << info;
  }

  network->Send(WRemoteTransmitMode::Reliable, msg);

  // make sure the message is out, before we shut down
  for (WUInt32 i = 0; i < 10; ++i)
  {
    network->UpdateRemoteInterface();
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));
  }

  network->ShutdownConnection();
  return W_SUCCESS;
}

void WFileserver::SetCustomMessageHandler(WUInt32 uiSystemID, ClientMessageHandler handler)
{
  m_CustomMessageHandlers[uiSystemID] = handler;
}

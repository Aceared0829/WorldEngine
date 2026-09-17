#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

W_IMPLEMENT_SINGLETON(WFileserveClient);

bool WFileserveClient::s_bEnableFileserve = true;

WFileserveClient::WFileserveClient()
  : m_SingletonRegistrar(this)
{
  AddServerAddressToTry("localhost:1042");

  WStringBuilder sAddress, sSearch;

  // the app directory
  {
    sSearch = WOSFile::GetApplicationDirectory();
    sSearch.AppendPath("WFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  // command line argument
  AddServerAddressToTry(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // last successful IP is stored in the user directory
  {
    sSearch = WOSFile::GetUserDataFolder("WFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  if (WCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_off"))
    s_bEnableFileserve = false;

  m_CurrentTime = WTime::Now();
}

WFileserveClient::~WFileserveClient()
{
  ShutdownConnection();
}

void WFileserveClient::ShutdownConnection()
{
  if (m_pNetwork)
  {
    WLog::Dev("Shutting down fileserve client");

    m_pNetwork->ShutdownConnection();
    m_pNetwork = nullptr;
  }
}

void WFileserveClient::ClearState()
{
  m_bDownloading = false;
  m_bWaitingForUploadFinished = false;
  m_CurFileRequestGuid = WUuid();
  m_sCurFileRequest.Clear();
  m_Download.Clear();
}

WResult WFileserveClient::EnsureConnected(WTime timeout)
{
  W_LOCK(m_Mutex);
  if (!s_bEnableFileserve || m_bFailedToConnect)
    return W_FAILURE;

  if (m_pNetwork == nullptr)
  {
    m_pNetwork = WRemoteInterfaceEnet::Make(); /// \todo Somehow abstract this away ?

    m_sFileserveCacheFolder = WOSFile::GetUserDataFolder("WFileserve/Cache");
    m_sFileserveCacheMetaFolder = WOSFile::GetUserDataFolder("WFileserve/Meta");

    if (WOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
    {
      WLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
      return W_FAILURE;
    }

    if (WOSFile::CreateDirectoryStructure(m_sFileserveCacheMetaFolder).Failed())
    {
      WLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheMetaFolder);
      return W_FAILURE;
    }
  }

  if (!m_pNetwork->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_pNetwork->ConnectToServer('EZFS', m_sServerConnectionAddress).Failed())
      return W_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = WTime::MakeFromSeconds(WCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }

    if (m_pNetwork->WaitForConnectionToServer(timeout).Failed())
    {
      m_pNetwork->ShutdownConnection();
      WLog::Error("Connection to WFileserver timed out");
      return W_FAILURE;
    }
    else
    {
      WLog::Success("Connected to WFileserver '{0}", m_sServerConnectionAddress);
      m_pNetwork->SetMessageHandler('FSRV', WMakeDelegate(&WFileserveClient::NetworkMsgHandler, this));

      m_pNetwork->Send('FSRV', 'HELO'); // be friendly
    }

    m_bFailedToConnect = false;
  }

  return W_SUCCESS;
}

void WFileserveClient::UpdateClient()
{
  W_LOCK(m_Mutex);
  if (m_pNetwork == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
    return;

  if (!m_pNetwork->IsConnectedToServer())
  {
    if (EnsureConnected().Failed())
    {
      WLog::Error("Fileserve connection was lost and could not be re-established.");
      ShutdownConnection();
    }
    return;
  }

  m_CurrentTime = WTime::Now();

  m_pNetwork->ExecuteAllMessageHandlers();
}

void WFileserveClient::AddServerAddressToTry(WStringView sAddress)
{
  W_LOCK(m_Mutex);
  if (sAddress.IsEmpty())
    return;

  if (m_TryServerAddresses.Contains(sAddress))
    return;

  m_TryServerAddresses.PushBack(sAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = sAddress;
}

void WFileserveClient::UploadFile(WUInt16 uiDataDirID, const char* szFile, const WDynamicArray<WUInt8>& fileContent)
{
  W_LOCK(m_Mutex);

  if (m_pNetwork == nullptr)
    return;

  // update meta state and cache
  {
    const WString& sMountPoint = m_MountedDataDirs[uiDataDirID].m_sMountPoint;
    WStringBuilder sCachedMetaFile;
    BuildPathInCache(szFile, sMountPoint, nullptr, &sCachedMetaFile);

    WUInt64 uiHash = 1;

    if (!fileContent.IsEmpty())
    {
      uiHash = WHashingUtils::xxHash64(fileContent.GetData(), fileContent.GetCount(), uiHash);
    }

    WriteMetaFile(sCachedMetaFile, 0, uiHash);

    InvalidateFileCache(uiDataDirID, szFile, uiHash);
  }

  const WUInt32 uiFileSize = fileContent.GetCount();

  WUuid uploadGuid = WUuid::MakeUuid();

  {
    WRemoteMessage msg;
    msg.SetMessageID('FSRV', 'UPLH');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiFileSize;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;
    m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);
  }

  WUInt32 uiNextByte = 0;

  // send the file over in multiple packages of 1KB each
  // send at least one package, even for empty files

  while (uiNextByte < fileContent.GetCount())
  {
    const WUInt16 uiChunkSize = (WUInt16)WMath::Min<WUInt32>(1024, fileContent.GetCount() - uiNextByte);

    WRemoteMessage msg;
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiChunkSize;
    msg.GetWriter().WriteBytes(&fileContent[uiNextByte], uiChunkSize).IgnoreResult();

    msg.SetMessageID('FSRV', 'UPLD');
    m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);

    uiNextByte += uiChunkSize;
  }

  // continuously update the network until we know the server has received the big chunk of data
  m_bWaitingForUploadFinished = true;

  // final message to server
  {
    WRemoteMessage msg('FSRV', 'UPLF');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;

    m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished)
  {
    UpdateClient();
  }
}


void WFileserveClient::InvalidateFileCache(WUInt16 uiDataDirID, WStringView sFile, WUInt64 uiHash)
{
  W_LOCK(m_Mutex);
  auto& cache = m_MountedDataDirs[uiDataDirID].m_CacheStatus[sFile];
  cache.m_FileHash = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck = WTime::MakeZero(); // will trigger a server request and that in turn will update the file timestamp

  // redirect the next access to this cache entry
  // together with the zero LastCheck that will make sure the best match gets updated as well
  m_FileDataDir[sFile] = uiDataDirID;
}

void WFileserveClient::FillFileStatusCache(const char* szFile)
{
  W_LOCK(m_Mutex);
  auto it = m_FileDataDir.FindOrAdd(szFile);
  it.Value() = 0xffff; // does not exist

  for (WUInt16 i = static_cast<WUInt16>(m_MountedDataDirs.GetCount()); i > 0; --i)
  {
    const WUInt16 dd = i - 1;

    if (!m_MountedDataDirs[dd].m_bMounted)
      continue;

    auto& cache = m_MountedDataDirs[dd].m_CacheStatus[szFile];

    DetermineCacheStatus(dd, szFile, cache);
    cache.m_LastCheck = WTime::MakeZero();

    if (cache.m_TimeStamp != 0 && cache.m_FileHash != 0) // file exists
    {
      // best possible candidate
      if (it.Value() == 0xffff)
        it.Value() = dd;
    }
  }

  if (it.Value() == 0xffff)
    it.Value() = 0; // fallback
}

void WFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, WStringBuilder* out_pAbsPath, WStringBuilder* out_pFullPathMeta) const
{
  W_ASSERT_DEV(!WPathUtils::IsAbsolutePath(szFile), "Invalid path");
  W_LOCK(m_Mutex);
  if (out_pAbsPath)
  {
    *out_pAbsPath = m_sFileserveCacheFolder;
    out_pAbsPath->AppendPath(szMountPoint, szFile);
    out_pAbsPath->MakeCleanPath();
  }
  if (out_pFullPathMeta)
  {
    *out_pFullPathMeta = m_sFileserveCacheMetaFolder;
    out_pFullPathMeta->AppendPath(szMountPoint, szFile);
    out_pFullPathMeta->MakeCleanPath();
  }
}

void WFileserveClient::ComputeDataDirMountPoint(WStringView sDataDir, WStringBuilder& out_sMountPoint)
{
  W_ASSERT_DEV(sDataDir.IsEmpty() || sDataDir.EndsWith("/"), "Invalid path");

  const WUInt32 uiMountPoint = WHashingUtils::xxHash32String(sDataDir);
  out_sMountPoint.SetFormat("{0}", WArgU(uiMountPoint, 8, true, 16));
}

void WFileserveClient::GetFullDataDirCachePath(const char* szDataDir, WStringBuilder& out_sFullPath, WStringBuilder& out_sFullPathMeta) const
{
  W_LOCK(m_Mutex);
  WStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(sMountPoint);
}

void WFileserveClient::NetworkMsgHandler(WRemoteMessage& msg)
{
  W_LOCK(m_Mutex);
  if (msg.GetMessageID() == 'DWNL')
  {
    HandleFileTransferMsg(msg);
    return;
  }

  if (msg.GetMessageID() == 'DWNF')
  {
    HandleFileTransferFinishedMsg(msg);
    return;
  }

  static bool s_bReloadResources = false;

  if (msg.GetMessageID() == 'RLDR')
  {
    s_bReloadResources = true;
  }

  if (!m_bDownloading && s_bReloadResources)
  {
    W_BROADCAST_EVENT(WResourceManager_ReloadAllResources);
    s_bReloadResources = false;
    return;
  }

  if (msg.GetMessageID() == 'RLDR')
    return;

  if (msg.GetMessageID() == 'UACK')
  {
    m_bWaitingForUploadFinished = false;
    return;
  }

  if (msg.GetMessageID() == 'INVC')
  {
    // invalidate caches, so that next read will go to the server

    for (auto& dd : m_MountedDataDirs)
    {
      for (auto& it : dd.m_CacheStatus)
      {
        it.Value().m_LastCheck = WTime::MakeZero();
      }
    }

    return;
  }

  WLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

WUInt16 WFileserveClient::MountDataDirectory(WStringView sDataDirectory, WStringView sRootName)
{
  W_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return 0xffff;

  WStringBuilder sRoot = sRootName;
  sRoot.Trim(":/");

  WStringBuilder sMountPoint;
  ComputeDataDirMountPoint(sDataDirectory, sMountPoint);

  const WUInt16 uiDataDirID = static_cast<WUInt16>(m_MountedDataDirs.GetCount());

  WRemoteMessage msg('FSRV', ' MNT');
  msg.GetWriter() << sDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  // dd.m_sPathOnClient = sDataDirectory;
  // dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted = true;

  return uiDataDirID;
}


void WFileserveClient::UnmountDataDirectory(WUInt16 uiDataDir)
{
  W_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  WRemoteMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void WFileserveClient::DeleteFile(WUInt16 uiDataDir, WStringView sFile)
{
  W_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  WRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);
}

void WFileserveClient::HandleFileTransferMsg(WRemoteMessage& msg)
{
  W_LOCK(m_Mutex);
  {
    WUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // WLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  WUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  WUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  // make sure we don't need to reallocate
  m_Download.Reserve(uiFileSize);

  if (uiChunkSize > 0)
  {
    const WUInt32 uiStartPos = m_Download.GetCount();
    m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
    msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
  }
}


void WFileserveClient::HandleFileTransferFinishedMsg(WRemoteMessage& msg)
{
  W_LOCK(m_Mutex);
  W_SCOPE_EXIT(m_bDownloading = false);

  {
    WUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // WLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  WFileserveFileState fileState;
  {
    WInt8 iFileStatus = 0;
    msg.GetReader() >> iFileStatus;
    fileState = (WFileserveFileState)iFileStatus;
  }

  WInt64 iFileTimeStamp = 0;
  msg.GetReader() >> iFileTimeStamp;

  WUInt64 uiFileHash = 0;
  msg.GetReader() >> uiFileHash;

  WUInt16 uiFoundInDataDir = 0;
  msg.GetReader() >> uiFoundInDataDir;

  if (uiFoundInDataDir == 0xffff)         // file does not exist on server in any data dir
  {
    m_FileDataDir[m_sCurFileRequest] = 0; // placeholder

    for (WUInt32 i = 0; i < m_MountedDataDirs.GetCount(); ++i)
    {
      auto& ref = m_MountedDataDirs[i].m_CacheStatus[m_sCurFileRequest];
      ref.m_FileHash = 0;
      ref.m_TimeStamp = 0;
      ref.m_LastCheck = m_CurrentTime;
    }

    return;
  }
  else
  {
    m_FileDataDir[m_sCurFileRequest] = uiFoundInDataDir;

    auto& ref = m_MountedDataDirs[uiFoundInDataDir].m_CacheStatus[m_sCurFileRequest];
    ref.m_FileHash = uiFileHash;
    ref.m_TimeStamp = iFileTimeStamp;
    ref.m_LastCheck = m_CurrentTime;
  }

  // nothing changed
  if (fileState == WFileserveFileState::SameTimestamp || fileState == WFileserveFileState::NonExistantEither)
    return;

  const WString& sMountPoint = m_MountedDataDirs[uiFoundInDataDir].m_sMountPoint;
  WStringBuilder sCachedFile, sCachedMetaFile;
  BuildPathInCache(m_sCurFileRequest, sMountPoint, &sCachedFile, &sCachedMetaFile);

  if (fileState == WFileserveFileState::NonExistant)
  {
    // remove them from the cache as well, if they still exist there
    WOSFile::DeleteFile(sCachedFile).IgnoreResult();
    WOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
    return;
  }

  // timestamp changed, but hash is still the same -> update timestamp
  if (fileState == WFileserveFileState::SameHash)
  {
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }

  if (fileState == WFileserveFileState::Different)
  {
    WriteDownloadToDisk(sCachedFile);
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }
}


void WFileserveClient::WriteMetaFile(WStringBuilder sCachedMetaFile, WInt64 iFileTimeStamp, WUInt64 uiFileHash)
{
  WOSFile file;
  if (file.Open(sCachedMetaFile, WFileOpenMode::Write).Succeeded())
  {
    file.Write(&iFileTimeStamp, sizeof(WInt64)).IgnoreResult();
    file.Write(&uiFileHash, sizeof(WUInt64)).IgnoreResult();

    file.Close();
  }
  else
  {
    WLog::Error("Failed to write meta file to '{0}'", sCachedMetaFile);
  }
}

void WFileserveClient::WriteDownloadToDisk(WStringBuilder sCachedFile)
{
  W_LOCK(m_Mutex);
  WOSFile file;
  if (file.Open(sCachedFile, WFileOpenMode::Write).Succeeded())
  {
    if (!m_Download.IsEmpty())
      file.Write(m_Download.GetData(), m_Download.GetCount()).IgnoreResult();

    file.Close();
  }
  else
  {
    WLog::Error("Failed to write download to '{0}'", sCachedFile);
  }
}

WResult WFileserveClient::DownloadFile(WUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, WStringBuilder* out_pFullPath)
{
  // bForceThisDataDir = true;
  W_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    WLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return W_FAILURE;
  }

  W_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  W_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  W_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_pNetwork->IsConnectedToServer())
    return W_FAILURE;

  bool bCachedYet = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const WUInt16 uiUseDataDirCache = bForceThisDataDir ? uiDataDirID : itFileDataDir.Value();
  const FileCacheStatus& CacheStatus = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (m_CurrentTime - CacheStatus.m_LastCheck < WTime::MakeFromSeconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0) // file does not exist
      return W_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiUseDataDirCache].m_sMountPoint, out_pFullPath, nullptr);

    return W_SUCCESS;
  }

  m_Download.Clear();
  m_sCurFileRequest = szFile;
  m_CurFileRequestGuid = WUuid::MakeUuid();
  m_bDownloading = true;

  WRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_pNetwork->Send(WRemoteTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_pNetwork->UpdateRemoteInterface();
    m_pNetwork->ExecuteAllMessageHandlers();
  }

  if (bForceThisDataDir)
  {
    if (m_MountedDataDirs[uiDataDirID].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
      return W_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiDataDirID].m_sMountPoint, out_pFullPath, nullptr);

    return W_SUCCESS;
  }
  else
  {
    const WUInt16 uiBestDir = itFileDataDir.Value();
    if (uiBestDir == uiDataDirID) // best match is still this? -> success
    {
      // file does not exist
      if (m_MountedDataDirs[uiBestDir].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
        return W_FAILURE;

      if (out_pFullPath)
        BuildPathInCache(szFile, m_MountedDataDirs[uiBestDir].m_sMountPoint, out_pFullPath, nullptr);

      return W_SUCCESS;
    }

    return W_FAILURE;
  }
}

void WFileserveClient::DetermineCacheStatus(WUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const
{
  W_LOCK(m_Mutex);
  WStringBuilder sAbsPathFile, sAbsPathMeta;
  const auto& dd = m_MountedDataDirs[uiDataDirID];

  W_ASSERT_DEV(dd.m_bMounted, "Data directory {0} is not mounted", uiDataDirID);

  BuildPathInCache(szFile, dd.m_sMountPoint, &sAbsPathFile, &sAbsPathMeta);

  if (WOSFile::ExistsFile(sAbsPathFile))
  {
    WOSFile meta;
    if (meta.Open(sAbsPathMeta, WFileOpenMode::Read).Failed())
    {
      // cleanup, when the meta file does not exist, the data file is useless
      WOSFile::DeleteFile(sAbsPathFile).IgnoreResult();
      return;
    }

    meta.Read(&out_Status.m_TimeStamp, sizeof(WInt64));
    meta.Read(&out_Status.m_FileHash, sizeof(WUInt64));
  }
}

WResult WFileserveClient::TryReadFileserveConfig(const char* szFile, WStringBuilder& out_Result)
{
  WOSFile file;
  if (file.Open(szFile, WFileOpenMode::Read).Succeeded())
  {
    WUInt8 data[64]; // an IP + port should not be longer than 22 characters

    WStringBuilder res;

    data[file.Read(data, 63)] = 0;
    res = (const char*)data;
    res.Trim(" \t\n\r");

    if (res.IsEmpty())
      return W_FAILURE;

    // has to contain a port number
    if (res.FindSubString(":") == nullptr)
      return W_FAILURE;

    // otherwise could be an arbitrary string
    out_Result = res;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WFileserveClient::SearchForServerAddress(WTime timeout /*= WTime::MakeFromSeconds(5)*/)
{
  W_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return W_FAILURE;

  WStringBuilder sAddress;

  // add the command line argument again, in case this was modified since the constructor ran
  // will not change anything, if this is a duplicate
  AddServerAddressToTry(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // go through the available options
  for (WInt32 idx = m_TryServerAddresses.GetCount() - 1; idx >= 0; --idx)
  {
    if (TryConnectWithFileserver(m_TryServerAddresses[idx], timeout).Succeeded())
      return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WFileserveClient::TryConnectWithFileserver(const char* szAddress, WTime timeout) const
{
  W_LOCK(m_Mutex);
  if (WStringUtils::IsNullOrEmpty(szAddress))
    return W_FAILURE;

  WLog::Info("File server address: '{0}' ({1} sec)", szAddress, timeout.GetSeconds());

  WUniquePtr<WRemoteInterfaceEnet> network = WRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
  if (network->ConnectToServer('EZFS', szAddress, false).Failed())
    return W_FAILURE;

  bool bServerFound = false;
  network->SetMessageHandler('FSRV', [&bServerFound](WRemoteMessage& ref_msg)
    {
    switch (ref_msg.GetMessageID())
    {
      case ' YES':
        bServerFound = true;
        break;
    } });

  if (network->WaitForConnectionToServer(timeout).Succeeded())
  {
    // wait for a proper response
    WTime tStart = WTime::Now();
    while (WTime::Now() - tStart < timeout && !bServerFound)
    {
      network->Send('FSRV', 'RUTR');

      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }
  }

  network->ShutdownConnection();

  if (!bServerFound)
    return W_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  SaveCurrentConnectionInfoToDisk().IgnoreResult();
  return W_SUCCESS;
}

WResult WFileserveClient::WaitForServerInfo(WTime timeout /*= WTime::MakeFromSeconds(60.0 * 5)*/)
{
  W_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return W_FAILURE;

  WUInt16 uiPort = 1042;
  WTempHybridArray<WStringBuilder, 4> sServerIPs;

  {
    WUniquePtr<WRemoteInterfaceEnet> network = WRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
    network->SetMessageHandler('FSRV', [&sServerIPs, &uiPort](WRemoteMessage& ref_msg)

      {
        switch (ref_msg.GetMessageID())
        {
          case 'MYIP':
            ref_msg.GetReader() >> uiPort;

            WUInt8 uiCount = 0;
            ref_msg.GetReader() >> uiCount;

            sServerIPs.SetCount(uiCount);
            for (WUInt32 i = 0; i < uiCount; ++i)
            {
              ref_msg.GetReader() >> sServerIPs[i];
            }

            break;
        } });

    W_SUCCEED_OR_RETURN(network->StartServer('EZIP', "2042", false));

    WTime tStart = WTime::Now();
    while (WTime::Now() - tStart < timeout && sServerIPs.IsEmpty())
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }

    network->ShutdownConnection();
  }

  if (sServerIPs.IsEmpty())
    return W_FAILURE;

  // network connections are unreliable and surprisingly slow sometimes
  // we just got an IP from a server, so we know it's there and we should be able to connect to it
  // still this often fails the first few times
  // so we try this several times and waste some time in between and hope that at some point the connection succeeds
  for (WUInt32 i = 0; i < 8; ++i)
  {
    WStringBuilder sAddress;
    for (auto& ip : sServerIPs)
    {
      sAddress.SetFormat("{0}:{1}", ip, uiPort);

      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(500));

      if (TryConnectWithFileserver(sAddress, WTime::MakeFromSeconds(3)).Succeeded())
        return W_SUCCESS;
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1000));
  }

  return W_FAILURE;
}

WResult WFileserveClient::SaveCurrentConnectionInfoToDisk() const
{
  W_LOCK(m_Mutex);
  WStringBuilder sFile = WOSFile::GetUserDataFolder("WFileserve.txt");
  WOSFile file;
  W_SUCCEED_OR_RETURN(file.Open(sFile, WFileOpenMode::Write));

  W_SUCCEED_OR_RETURN(file.Write(m_sServerConnectionAddress.GetData(), m_sServerConnectionAddress.GetElementCount()));
  file.Close();

  return W_SUCCESS;
}

W_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (WFileserveClient::GetSingleton())
  {
    WFileserveClient::GetSingleton()->UpdateClient();
  }
}



W_STATICLINK_FILE(FileServePlugin, FileServePlugin_Client_FileserveClient);

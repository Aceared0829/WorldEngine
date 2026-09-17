#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/Logging/Log.h>

void WDataDirectory::FileserveType::ReloadExternalConfigs()
{
  W_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (!s_sRedirectionFile.IsEmpty())
  {
    WFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, s_sRedirectionFile, true, nullptr).IgnoreResult();
  }

  FolderType::ReloadExternalConfigs();
}

WDataDirectoryReader* WDataDirectory::FileserveType::OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (WPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  WStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bSpecificallyThisDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (WConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  WStringBuilder sFullPath;
  if (WFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bSpecificallyThisDataDir, &sFullPath).Failed())
    return nullptr;

  // It's fine to use the base class here as it will resurface in CreateFolderReader which gives us control of the important part.
  return FolderType::OpenFileToRead(sFullPath, FileShareMode, bSpecificallyThisDataDir);
}

WDataDirectoryWriter* WDataDirectory::FileserveType::OpenFileToWrite(WStringView sFile, WFileShareMode::Enum FileShareMode)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (WPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  return FolderType::OpenFileToWrite(sFile, FileShareMode);
}

WResult WDataDirectory::FileserveType::InternalInitializeDataDirectory(WStringView sDirectory)
{
  WStringBuilder sDataDir = sDirectory;
  sDataDir.MakeCleanPath();

  WStringBuilder sCacheFolder, sCacheMetaFolder;
  WFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder, sCacheMetaFolder);
  m_sRedirectedDataDirPath = sCacheFolder;
  m_sFileserveCacheMetaFolder = sCacheMetaFolder;

  ReloadExternalConfigs();
  return W_SUCCESS;
}

void WDataDirectory::FileserveType::RemoveDataDirectory()
{
  if (WFileserveClient::GetSingleton())
  {
    WFileserveClient::GetSingleton()->UnmountDataDirectory(m_uiDataDirID);
  }

  FolderType::RemoveDataDirectory();
}

void WDataDirectory::FileserveType::DeleteFile(WStringView sFile)
{
  if (WFileserveClient::GetSingleton())
  {
    WFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, sFile);
  }

  FolderType::DeleteFile(sFile);
}

WDataDirectory::FolderReader* WDataDirectory::FileserveType::CreateFolderReader() const
{
  return W_DEFAULT_NEW(FileserveDataDirectoryReader, 0);
}

WDataDirectory::FolderWriter* WDataDirectory::FileserveType::CreateFolderWriter() const
{
  return W_DEFAULT_NEW(FileserveDataDirectoryWriter);
}

WResult WDataDirectory::FileserveType::GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats)
{
  WStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFileOrFolder, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (WConversionUtils::IsStringUuid(sRedirected))
    return W_FAILURE;

  WStringBuilder sFullPath;
  W_SUCCEED_OR_RETURN(WFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  return WOSFile::GetFileStats(sFullPath, out_Stats);
}

bool WDataDirectory::FileserveType::ExistsFile(WStringView sFile, bool bOneSpecificDataDir)
{
  WStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (WConversionUtils::IsStringUuid(sRedirected))
    return false;

  return WFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, nullptr).Succeeded();
}

WDataDirectoryType* WDataDirectory::FileserveType::Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage)
{
  if (!WFileserveClient::s_bEnableFileserve || WFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (sDataDirectory.IsEmpty())
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (!sDataDirectory.StartsWith(">"))
    return nullptr;

  if (WFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  WDataDirectory::FileserveType* pDataDir = W_DEFAULT_NEW(WDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID = WFileserveClient::GetSingleton()->MountDataDirectory(sDataDirectory, sRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(sDataDirectory) == W_SUCCESS)
    return pDataDir;

  W_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

WDataDirectory::FileserveDataDirectoryReader::FileserveDataDirectoryReader(WInt32 iDataDirUserData)
  : FolderReader(iDataDirUserData)
{
}

WResult WDataDirectory::FileserveDataDirectoryReader::InternalOpen(WFileShareMode::Enum FileShareMode)
{
  return m_File.Open(GetFilePath().GetData(), WFileOpenMode::Read, FileShareMode);
}

void WDataDirectory::FileserveDataDirectoryWriter::InternalClose()
{
  FolderWriter::InternalClose();

  static_cast<FileserveType*>(GetDataDirectory())->FinishedWriting(this);
}

void WDataDirectory::FileserveType::FinishedWriting(FolderWriter* pWriter)
{
  if (WFileserveClient::GetSingleton() == nullptr)
    return;

  WStringBuilder sAbsPath = pWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
  sAbsPath.AppendPath(pWriter->GetFilePath());

  WOSFile file;
  if (file.Open(sAbsPath, WFileOpenMode::Read).Failed())
  {
    WLog::Error("Could not read file for upload: '{0}'", sAbsPath);
    return;
  }

  WDynamicArray<WUInt8> content;
  file.ReadAll(content);
  file.Close();

  WFileserveClient::GetSingleton()->UploadFile(m_uiDataDirID, pWriter->GetFilePath(), content);
}



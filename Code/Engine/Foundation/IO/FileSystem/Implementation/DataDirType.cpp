#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

WResult WDataDirectoryType::InitializeDataDirectory(WStringView sDataDirPath)
{
  WStringBuilder sPath = sDataDirPath;
  sPath.MakeCleanPath();

  W_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool WDataDirectoryType::ExistsFile(WStringView sFile, bool bOneSpecificDataDir)
{
  W_IGNORE_UNUSED(bOneSpecificDataDir);

  WStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(sFile, sRedirectedAsset);

  WStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return WOSFile::ExistsFile(sPath);
}

void WDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  WFileSystem::FileEvent fe;
  fe.m_EventType = WFileSystem::FileEventType::CloseFile;
  fe.m_sFileOrDirectory = GetFilePath();
  fe.m_pDataDir = m_pDataDirType;
  WFileSystem::s_pData->m_Event.Broadcast(fe);

  m_pDataDirType->OnReaderWriterClose(this);
}

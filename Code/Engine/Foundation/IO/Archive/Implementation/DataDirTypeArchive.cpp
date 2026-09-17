#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ArchiveDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem", "FolderDataDirectory"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::ArchiveType::Factory);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WDataDirectory::ArchiveType::ArchiveType() = default;
WDataDirectory::ArchiveType::~ArchiveType() = default;

WDataDirectoryType* WDataDirectory::ArchiveType::Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage)
{
  W_IGNORE_UNUSED(sGroup);
  W_IGNORE_UNUSED(sRootName);
  W_IGNORE_UNUSED(usage);

  ArchiveType* pDataDir = W_DEFAULT_NEW(ArchiveType);

  if (pDataDir->InitializeDataDirectory(sDataDirectory) == W_SUCCESS)
    return pDataDir;

  W_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

WDataDirectoryReader* WDataDirectory::ArchiveType::OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  W_IGNORE_UNUSED(bSpecificallyThisDataDir);

  const WArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  WStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);
  sArchivePath.MakeCleanPath();

  const WUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == WInvalidIndex)
    return nullptr;

  const WArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  ArchiveReaderCommon* pReader = nullptr;

  {
    W_LOCK(m_ReaderMutex);

    switch (pEntry->m_CompressionMode)
    {
      case WArchiveCompressionMode::Uncompressed:
      {
        if (!m_FreeReadersUncompressed.IsEmpty())
        {
          pReader = m_FreeReadersUncompressed.PeekBack();
          m_FreeReadersUncompressed.PopBack();
        }
        else
        {
          m_ReadersUncompressed.PushBack(W_DEFAULT_NEW(ArchiveReaderUncompressed, 0));
          pReader = m_ReadersUncompressed.PeekBack().Borrow();
        }
        break;
      }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      case WArchiveCompressionMode::Compressed_zstd:
      {
        if (!m_FreeReadersZstd.IsEmpty())
        {
          pReader = m_FreeReadersZstd.PeekBack();
          m_FreeReadersZstd.PopBack();
        }
        else
        {
          m_ReadersZstd.PushBack(W_DEFAULT_NEW(ArchiveReaderZstd, 1));
          pReader = m_ReadersZstd.PeekBack().Borrow();
        }
        break;
      }
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
      case WArchiveCompressionMode::Compressed_zip:
      {
        if (!m_FreeReadersZip.IsEmpty())
        {
          pReader = m_FreeReadersZip.PeekBack();
          m_FreeReadersZip.PopBack();
        }
        else
        {
          m_ReadersZip.PushBack(W_DEFAULT_NEW(ArchiveReaderZip, 2));
          pReader = m_ReadersZip.PeekBack().Borrow();
        }
        break;
      }
#endif

      default:
        W_REPORT_FAILURE("Compression mode {} is unknown (or not compiled in)", (WUInt8)pEntry->m_CompressionMode);
        return nullptr;
    }
  }

  pReader->m_uiUncompressedSize = pEntry->m_uiUncompressedDataSize;
  pReader->m_uiCompressedSize = pEntry->m_uiStoredDataSize;

  m_ArchiveReader.ConfigureRawMemoryStreamReader(uiEntryIndex, pReader->m_MemStreamReader);

  if (pReader->Open(sArchivePath, this, FileShareMode).Failed())
  {
    W_DEFAULT_DELETE(pReader);
    return nullptr;
  }

  return pReader;
}

void WDataDirectory::ArchiveType::RemoveDataDirectory()
{
  ArchiveType* pThis = this;
  W_DEFAULT_DELETE(pThis);
}

bool WDataDirectory::ArchiveType::ExistsFile(WStringView sFile, bool bOneSpecificDataDir)
{
  W_IGNORE_UNUSED(bOneSpecificDataDir);

  WStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);
  sArchivePath.MakeCleanPath();
  return m_ArchiveReader.GetArchiveTOC().FindEntry(sArchivePath) != WInvalidIndex;
}

WResult WDataDirectory::ArchiveType::GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats)
{
  W_IGNORE_UNUSED(bOneSpecificDataDir);

  const WArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  WStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFileOrFolder);
  // We might be called with paths like AAA/../BBB which we won't find in the toc unless we clean the path first.
  sArchivePath.MakeCleanPath();
  const WUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == WInvalidIndex)
    return W_FAILURE;

  const WArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  const WStringView sPath = toc.GetEntryPathString(uiEntryIndex);

  out_Stats.m_bIsDirectory = false;
  out_Stats.m_LastModificationTime = m_LastModificationTime;
  out_Stats.m_uiFileSize = pEntry->m_uiUncompressedDataSize;
  out_Stats.m_sParentPath = sPath;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = WPathUtils::GetFileNameAndExtension(sPath);

  return W_SUCCESS;
}

WResult WDataDirectory::ArchiveType::InternalInitializeDataDirectory(WStringView sDirectory)
{
  WStringBuilder sRedirected;
  W_SUCCEED_OR_RETURN(WFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected));

  sRedirected.MakeCleanPath();
  // remove trailing slashes
  sRedirected.Trim("", "/");
  m_sRedirectedDataDirPath = sRedirected;

  bool bSupported = false;
  WStringBuilder sArchivePath;

  WHybridArray<WString, 4, WStaticsAllocatorWrapper> extensions = WArchiveUtils::GetAcceptedArchiveFileExtensions();

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  extensions.PushBack("zip");
  extensions.PushBack("apk");
#endif

  for (const auto& ext : extensions)
  {
    const WUInt32 uiLength = ext.GetElementCount();
    if (sRedirected.HasExtension(ext))
    {
      sArchivePath = sRedirected;
      m_sArchiveSubFolder = "";
      bSupported = true;
      goto endloop;
    }
    const char* szFound = nullptr;
    do
    {
      szFound = sRedirected.FindLastSubString_NoCase(ext, szFound);
      if (szFound != nullptr && szFound[uiLength] == '/')
      {
        sArchivePath = WStringView(sRedirected.GetData(), szFound + uiLength);
        m_sArchiveSubFolder = szFound + uiLength + 1;
        bSupported = true;
        goto endloop;
      }

    } while (szFound != nullptr);
  }
endloop:
  if (!bSupported)
    return W_FAILURE;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats stats;
  if (WOSFile::GetFileStats(sArchivePath, stats).Failed())
    return W_FAILURE;
  m_LastModificationTime = stats.m_LastModificationTime;
#endif

  W_LOG_BLOCK("WArchiveDataDir", sDirectory);

  W_SUCCEED_OR_RETURN(m_ArchiveReader.OpenArchive(sArchivePath));

  ReloadExternalConfigs();

  return W_SUCCESS;
}

void WDataDirectory::ArchiveType::OnReaderWriterClose(WDataDirectoryReaderWriterBase* pClosed)
{
  W_LOCK(m_ReaderMutex);

  if (pClosed->GetDataDirUserData() == 0)
  {
    m_FreeReadersUncompressed.PushBack(static_cast<ArchiveReaderUncompressed*>(pClosed));
    return;
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (pClosed->GetDataDirUserData() == 1)
  {
    m_FreeReadersZstd.PushBack(static_cast<ArchiveReaderZstd*>(pClosed));
    return;
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  if (pClosed->GetDataDirUserData() == 2)
  {
    m_FreeReadersZip.PushBack(static_cast<ArchiveReaderZip*>(pClosed));
    return;
  }
#endif

  W_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

WDataDirectory::ArchiveReaderCommon::ArchiveReaderCommon(WInt32 iDataDirUserData)
  : WDataDirectoryReader(iDataDirUserData)
{
}

WUInt64 WDataDirectory::ArchiveReaderCommon::GetFileSize() const
{
  return m_uiUncompressedSize;
}

//////////////////////////////////////////////////////////////////////////

WDataDirectory::ArchiveReaderUncompressed::ArchiveReaderUncompressed(WInt32 iDataDirUserData)
  : ArchiveReaderCommon(iDataDirUserData)
{
}

WUInt64 WDataDirectory::ArchiveReaderUncompressed::Skip(WUInt64 uiBytes)
{
  return m_MemStreamReader.SkipBytes(uiBytes);
}

WUInt64 WDataDirectory::ArchiveReaderUncompressed::Read(void* pBuffer, WUInt64 uiBytes)
{
  return m_MemStreamReader.ReadBytes(pBuffer, uiBytes);
}

WResult WDataDirectory::ArchiveReaderUncompressed::InternalOpen(WFileShareMode::Enum FileShareMode)
{
  W_IGNORE_UNUSED(FileShareMode);
  W_ASSERT_DEBUG(FileShareMode != WFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  // nothing to do
  return W_SUCCESS;
}

void WDataDirectory::ArchiveReaderUncompressed::InternalClose()
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

WDataDirectory::ArchiveReaderZstd::ArchiveReaderZstd(WInt32 iDataDirUserData)
  : ArchiveReaderCommon(iDataDirUserData)
{
}

WUInt64 WDataDirectory::ArchiveReaderZstd::Read(void* pBuffer, WUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

WResult WDataDirectory::ArchiveReaderZstd::InternalOpen(WFileShareMode::Enum FileShareMode)
{
  W_IGNORE_UNUSED(FileShareMode);
  W_ASSERT_DEBUG(FileShareMode != WFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader);
  return W_SUCCESS;
}

void WDataDirectory::ArchiveReaderZstd::InternalClose()
{
  // nothing to do
}
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

WDataDirectory::ArchiveReaderZip::ArchiveReaderZip(WInt32 iDataDirUserData)
  : ArchiveReaderUncompressed(iDataDirUserData)
{
}

WDataDirectory::ArchiveReaderZip::~ArchiveReaderZip() = default;

WUInt64 WDataDirectory::ArchiveReaderZip::Read(void* pBuffer, WUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

WResult WDataDirectory::ArchiveReaderZip::InternalOpen(WFileShareMode::Enum FileShareMode)
{
  W_IGNORE_UNUSED(FileShareMode);
  W_ASSERT_DEBUG(FileShareMode != WFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader, m_uiCompressedSize);
  return W_SUCCESS;
}

#endif

W_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_DataDirTypeArchive);

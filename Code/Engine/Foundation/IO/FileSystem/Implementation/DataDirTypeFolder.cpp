#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FolderDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace WDataDirectory
{
  WString FolderType::s_sRedirectionFile;
  WString FolderType::s_sRedirectionPrefix;

  WResult FolderReader::InternalOpen(WFileShareMode::Enum FileShareMode)
  {
    WStringBuilder sPath = ((WDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), WFileOpenMode::Read, FileShareMode);
  }

  void FolderReader::InternalClose()
  {
    m_File.Close();
  }

  WUInt64 FolderReader::Skip(WUInt64 uiBytes)
  {
    if (uiBytes == 0)
    {
      return 0;
    }

    const WUInt64 fileSize = m_File.GetFileSize();
    const WUInt64 origFilePosition = m_File.GetFilePosition();
    W_ASSERT_DEBUG(origFilePosition <= fileSize, "");

    const WUInt64 newFilePosition = WMath::Min(fileSize, origFilePosition + uiBytes);
    m_File.SetFilePosition(newFilePosition, WFileSeekMode::FromStart);
    W_ASSERT_DEBUG(newFilePosition == m_File.GetFilePosition(), "");

    W_ASSERT_DEBUG(newFilePosition >= origFilePosition, "");
    return newFilePosition - origFilePosition;
  }

  WUInt64 FolderReader::Read(void* pBuffer, WUInt64 uiBytes)
  {
    return m_File.Read(pBuffer, uiBytes);
  }

  WUInt64 FolderReader::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  WResult FolderWriter::InternalOpen(WFileShareMode::Enum FileShareMode)
  {
    WStringBuilder sPath = ((WDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), WFileOpenMode::Write, FileShareMode);
  }

  void FolderWriter::InternalClose()
  {
    m_File.Close();
  }

  WResult FolderWriter::Write(const void* pBuffer, WUInt64 uiBytes)
  {
    return m_File.Write(pBuffer, uiBytes);
  }

  WUInt64 FolderWriter::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  WDataDirectoryType* FolderType::Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage)
  {
    W_IGNORE_UNUSED(sGroup);
    W_IGNORE_UNUSED(sRootName);
    W_IGNORE_UNUSED(usage);

    FolderType* pDataDir = W_DEFAULT_NEW(FolderType);

    if (pDataDir->InitializeDataDirectory(sDataDirectory) == W_SUCCESS)
      return pDataDir;

    W_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }

  void FolderType::RemoveDataDirectory()
  {
    {
      W_LOCK(m_ReaderWriterMutex);
      for (WUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        W_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }

      for (WUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        W_ASSERT_DEV(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }
    }
    FolderType* pThis = this;
    W_DEFAULT_DELETE(pThis);
  }

  void FolderType::DeleteFile(WStringView sFile)
  {
    WStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sFile);

    WOSFile::DeleteFile(sPath.GetData()).IgnoreResult();
  }

  FolderType::~FolderType()
  {
    W_LOCK(m_ReaderWriterMutex);
    for (WUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      W_DEFAULT_DELETE(m_Readers[i]);

    for (WUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      W_DEFAULT_DELETE(m_Writers[i]);
  }

  void FolderType::ReloadExternalConfigs()
  {
    LoadRedirectionFile();
  }

  void FolderType::LoadRedirectionFile()
  {
    W_LOCK(m_RedirectionMutex);
    m_FileRedirection.Clear();

    if (!s_sRedirectionFile.IsEmpty())
    {
      WStringBuilder sRedirectionFile(GetRedirectedDataDirectoryPath(), "/", s_sRedirectionFile);
      sRedirectionFile.MakeCleanPath();

      W_LOG_BLOCK("LoadRedirectionFile", sRedirectionFile.GetData());

      WOSFile file;
      if (file.Open(sRedirectionFile, WFileOpenMode::Read).Succeeded())
      {
        WTempHybridArray<char, 1024 * 10> content;
        content.Reserve((WUInt32)(file.GetFileSize() + 1));
        char uiTemp[4096];

        WUInt64 uiRead = 0;

        do
        {
          uiRead = file.Read(uiTemp, W_ARRAY_SIZE(uiTemp));
          content.PushBackRange(WArrayPtr<char>(uiTemp, (WUInt32)uiRead));
        } while (uiRead == W_ARRAY_SIZE(uiTemp));

        content.PushBack(0); // make sure the string is terminated

        const char* szLineStart = content.GetData();
        const char* szSeparator = nullptr;
        const char* szLineEnd = nullptr;

        WStringBuilder sFileToRedirect, sRedirection;

        while (true)
        {
          szSeparator = WStringUtils::FindSubString(szLineStart, ";");
          szLineEnd = WStringUtils::FindSubString(szSeparator, "\n");

          if (szLineStart == nullptr || szSeparator == nullptr || szLineEnd == nullptr)
            break;

          sFileToRedirect.SetSubString_FromTo(szLineStart, szSeparator);
          sRedirection.SetSubString_FromTo(szSeparator + 1, szLineEnd);

          m_FileRedirection[sFileToRedirect] = sRedirection;

          szLineStart = szLineEnd + 1;
        }

        // WLog::Debug("Redirection file contains {0} entries", m_FileRedirection.GetCount());
      }
      // else
      // WLog::Debug("No Redirection file found in: '{0}'", sRedirectionFile);
    }
  }


  bool FolderType::ExistsFile(WStringView sFile, bool bOneSpecificDataDir)
  {
    W_IGNORE_UNUSED(bOneSpecificDataDir);

    WStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFile, sRedirectedAsset);

    WStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sRedirectedAsset);
    sPath.MakeCleanPath();

    return sPath.IsAbsolutePath() && WOSFile::ExistsFile(sPath);
  }

  WResult FolderType::GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats)
  {
    W_IGNORE_UNUSED(bOneSpecificDataDir);

    WStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFileOrFolder, sRedirectedAsset);

    WStringBuilder sPath = GetRedirectedDataDirectoryPath();

    if (WPathUtils::IsAbsolutePath(sRedirectedAsset))
    {
      if (!sRedirectedAsset.StartsWith_NoCase(sPath))
        return W_FAILURE;

      sPath.Clear();
    }

    sPath.AppendPath(sRedirectedAsset);

    if (!WPathUtils::IsAbsolutePath(sPath))
      return W_FAILURE;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
    return WOSFile::GetFileStats(sPath, out_Stats);
#else
    return W_FAILURE;
#endif
  }

  WResult FolderType::InternalInitializeDataDirectory(WStringView sDirectory)
  {
    // allow to set the 'empty' directory to handle all absolute paths
    if (sDirectory.IsEmpty())
      return W_SUCCESS;

    WStringBuilder sRedirected;
    if (WFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected).Succeeded())
    {
      m_sRedirectedDataDirPath = sRedirected;
    }
    else
    {
      m_sRedirectedDataDirPath = sDirectory;
    }

    if (!m_sRedirectedDataDirPath.IsAbsolutePath() || !WOSFile::ExistsDirectory(m_sRedirectedDataDirPath))
      return W_FAILURE;

    ReloadExternalConfigs();

    return W_SUCCESS;
  }

  void FolderType::OnReaderWriterClose(WDataDirectoryReaderWriterBase* pClosed)
  {
    W_LOCK(m_ReaderWriterMutex);
    if (pClosed->IsReader())
    {
      FolderReader* pReader = (FolderReader*)pClosed;
      pReader->m_bIsInUse = false;
    }
    else
    {
      FolderWriter* pWriter = (FolderWriter*)pClosed;
      pWriter->m_bIsInUse = false;
    }
  }

  WDataDirectory::FolderReader* FolderType::CreateFolderReader() const
  {
    return W_DEFAULT_NEW(FolderReader, 0);
  }

  WDataDirectory::FolderWriter* FolderType::CreateFolderWriter() const
  {
    return W_DEFAULT_NEW(FolderWriter, 0);
  }

  WDataDirectoryReader* FolderType::OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
  {
    W_IGNORE_UNUSED(bSpecificallyThisDataDir);

    WStringBuilder sFileToOpen;
    ResolveAssetRedirection(sFile, sFileToOpen);

    // we know that these files cannot be opened, so don't even try
    if (WConversionUtils::IsStringUuid(sFileToOpen))
      return nullptr;

    FolderReader* pReader = nullptr;
    {
      W_LOCK(m_ReaderWriterMutex);
      for (WUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        if (!m_Readers[i]->m_bIsInUse)
          pReader = m_Readers[i];
      }

      if (pReader == nullptr)
      {
        m_Readers.PushBack(CreateFolderReader());
        pReader = m_Readers.PeekBack();
      }
      pReader->m_bIsInUse = true;
    }

    // if opening the file fails, the reader's m_bIsInUse needs to be reset.
    if (pReader->Open(sFileToOpen, this, FileShareMode) == W_FAILURE)
    {
      W_LOCK(m_ReaderWriterMutex);
      pReader->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pReader;
  }


  bool FolderType::ResolveAssetRedirection(WStringView sFile, WStringBuilder& out_sRedirection)
  {
    W_LOCK(m_RedirectionMutex);
    // Check if we know about a file redirection for this
    auto it = m_FileRedirection.Find(sFile);

    // if available, open the file that is mentioned in the redirection file instead
    if (it.IsValid())
    {

      if (it.Value().StartsWith("?"))
      {
        // ? is an option to tell the system to skip the redirection prefix and use the path as is
        out_sRedirection = &it.Value().GetData()[1];
      }
      else
      {
        out_sRedirection.Set(s_sRedirectionPrefix, it.Value());
      }
      return true;
    }
    else
    {
      out_sRedirection = sFile;
      return false;
    }
  }

  WDataDirectoryWriter* FolderType::OpenFileToWrite(WStringView sFile, WFileShareMode::Enum FileShareMode)
  {
    FolderWriter* pWriter = nullptr;

    {
      W_LOCK(m_ReaderWriterMutex);
      for (WUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        if (!m_Writers[i]->m_bIsInUse)
          pWriter = m_Writers[i];
      }

      if (pWriter == nullptr)
      {
        m_Writers.PushBack(CreateFolderWriter());
        pWriter = m_Writers.PeekBack();
      }
      pWriter->m_bIsInUse = true;
    }
    // if opening the file fails, the writer's m_bIsInUse needs to be reset.
    if (pWriter->Open(sFile, this, FileShareMode) == W_FAILURE)
    {
      W_LOCK(m_ReaderWriterMutex);
      pWriter->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pWriter;
  }
} // namespace WDataDirectory



W_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeFolder);

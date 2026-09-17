#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

WString64 WOSFile::s_sApplicationPath;
WString64 WOSFile::s_sUserDataPath;
WString64 WOSFile::s_sTempDataPath;
WString64 WOSFile::s_sUserDocumentsPath;
WAtomicInteger32 WOSFile::s_iFileCounter;

WOSFile::Event WOSFile::s_FileEvents;

WFileStats::WFileStats() = default;
WFileStats::~WFileStats() = default;

void WFileStats::GetFullPath(WStringBuilder& ref_sPath) const
{
  ref_sPath.Set(m_sParentPath, "/", m_sName);
  ref_sPath.MakeCleanPath();
}

WOSFile::WOSFile()
{
  m_FileMode = WFileOpenMode::None;
  m_iFileID = s_iFileCounter.Increment();
}

WOSFile::~WOSFile()
{
  Close();
}

WResult WOSFile::Open(WStringView sFile, WFileOpenMode::Enum openMode, WFileShareMode::Enum fileShareMode)
{
  m_iFileID = s_iFileCounter.Increment();

  W_ASSERT_DEV(openMode >= WFileOpenMode::Read && openMode <= WFileOpenMode::Append, "Invalid Mode");
  W_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  m_sFileName = sFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  WResult Res = W_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto done;

  {
    WStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (openMode == WFileOpenMode::Write || openMode == WFileOpenMode::Append)
    {
      W_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), openMode, fileShareMode) == W_SUCCESS)
  {
    m_FileMode = openMode;
    Res = W_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = WFileOpenMode::None;
  goto done;

done:

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_FileMode = openMode;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileOpen;

  s_FileEvents.Broadcast(e);

  return Res;
}

bool WOSFile::IsOpen() const
{
  return m_FileMode != WFileOpenMode::None;
}

void WOSFile::Close()
{
  if (!IsOpen())
    return;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  InternalClose();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = true;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileClose;

  s_FileEvents.Broadcast(e);

  m_sFileName.Clear();
  m_FileMode = WFileOpenMode::None;
}

WResult WOSFile::Write(const void* pBuffer, WUInt64 uiBytes)
{
  if (uiBytes == 0)
    return W_SUCCESS;

  W_ASSERT_DEV((m_FileMode == WFileOpenMode::Write) || (m_FileMode == WFileOpenMode::Append), "The file is not opened for writing.");
  W_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  const WResult Res = InternalWrite(pBuffer, uiBytes);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileWrite;
  e.m_uiBytesAccessed = uiBytes;

  s_FileEvents.Broadcast(e);

  return Res;
}

WUInt64 WOSFile::Read(void* pBuffer, WUInt64 uiBytes)
{
  W_ASSERT_DEV(m_FileMode == WFileOpenMode::Read, "The file is not opened for reading.");
  W_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  const WUInt64 Res = InternalRead(pBuffer, uiBytes);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = (Res == uiBytes);
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileRead;
  e.m_uiBytesAccessed = Res;

  s_FileEvents.Broadcast(e);

  return Res;
}

WUInt64 WOSFile::ReadAll(WDynamicArray<WUInt8>& out_fileContent)
{
  W_ASSERT_DEV(m_FileMode == WFileOpenMode::Read, "The file is not opened for reading.");

  out_fileContent.Clear();
  out_fileContent.SetCountUninitialized((WUInt32)GetFileSize());

  if (!out_fileContent.IsEmpty())
  {
    Read(out_fileContent.GetData(), out_fileContent.GetCount());
  }

  return out_fileContent.GetCount();
}

WUInt64 WOSFile::GetFilePosition() const
{
  W_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void WOSFile::SetFilePosition(WInt64 iDistance, WFileSeekMode::Enum pos) const
{
  W_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  W_ASSERT_DEV(m_FileMode != WFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, pos);
}

WUInt64 WOSFile::GetFileSize() const
{
  W_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

  const WInt64 iCurPos = static_cast<WInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, WFileSeekMode::FromEnd);

  const WUInt64 uiCurSize = static_cast<WInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, WFileSeekMode::FromStart);

  return uiCurSize;
}

const WString WOSFile::MakePathAbsoluteWithCWD(WStringView sPath)
{
  WStringBuilder tmp = sPath;
  tmp.MakeCleanPath();

  if (tmp.IsRelativePath())
  {
    tmp.PrependFormat("{}/", GetCurrentWorkingDirectory());
    tmp.MakeCleanPath();
  }

  return tmp;
}

bool WOSFile::ExistsFile(WStringView sFile)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  WStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  W_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute: '{}'", sFile);

  const bool bRes = InternalExistsFile(s);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif


  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = s;
  e.m_EventType = EventType::FileExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

bool WOSFile::ExistsDirectory(WStringView sDirectory)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  WStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  W_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute: '{}'", sDirectory);

  const bool bRes = InternalExistsDirectory(s);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = s;
  e.m_EventType = EventType::DirectoryExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

void WOSFile::FindFreeFilename(WStringBuilder& inout_sPath, WStringView sSuffix /*= {}*/)
{
  W_ASSERT_DEV(!inout_sPath.IsEmpty() && inout_sPath.IsAbsolutePath(), "Invalid input path.");

  if (!WOSFile::ExistsFile(inout_sPath))
    return;

  const WString orgName = inout_sPath.GetFileName();

  WStringBuilder newName;

  for (WUInt32 i = 2; i < 100000; ++i)
  {
    newName.SetFormat("{}{}{}", orgName, sSuffix, i);

    inout_sPath.ChangeFileName(newName);
    if (!WOSFile::ExistsFile(inout_sPath))
      return;
  }

  W_REPORT_FAILURE("Something went wrong.");
}

WResult WOSFile::DeleteFile(WStringView sFile)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  WStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const WResult Res = InternalDeleteFile(s.GetData());

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif
  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFile;
  e.m_EventType = EventType::FileDelete;

  s_FileEvents.Broadcast(e);

  return Res;
}

WStringView WOSFile::GetApplicationDirectory()
{
  if (s_sApplicationPath.IsEmpty())
  {
    // s_sApplicationPath is filled out and cached by GetApplicationPath(), so call that first, if necessary
    GetApplicationPath();
    W_ASSERT_ALWAYS(!s_sApplicationPath.IsEmpty(), "Invalid application directory");
  }

  return s_sApplicationPath.GetFileDirectory();
}

WResult WOSFile::CreateDirectoryStructure(WStringView sDirectory)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  WStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  W_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  WStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  WResult Res = W_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!WPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == W_FAILURE)
    {
      Res = W_FAILURE;
      break;
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sDirectory;
  e.m_EventType = EventType::MakeDir;

  s_FileEvents.Broadcast(e);

  return Res;
}

WResult WOSFile::MoveFileOrDirectory(WStringView sDirectoryFrom, WStringView sDirectoryTo)
{
  WStringBuilder sFrom(sDirectoryFrom);
  sFrom.MakeCleanPath();
  sFrom.MakePathSeparatorsNative();

  WStringBuilder sTo(sDirectoryTo);
  sTo.MakeCleanPath();
  sTo.MakePathSeparatorsNative();

  return InternalMoveFileOrDirectory(sFrom, sTo);
}

WResult WOSFile::CopyFile(WStringView sSource, WStringView sDestination)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#endif

  WOSFile SrcFile, DstFile;

  WResult Res = W_FAILURE;

  if (SrcFile.Open(sSource, WFileOpenMode::Read) == W_FAILURE)
    goto done;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(sDestination, WFileOpenMode::Write) == W_FAILURE)
    goto done;

  {
    const WUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

    // can't allocate that much data on the stack
    WDynamicArray<WUInt8> TempBuffer;
    TempBuffer.SetCountUninitialized(uiTempSize);

    while (true)
    {
      const WUInt64 uiRead = SrcFile.Read(&TempBuffer[0], uiTempSize);

      if (uiRead == 0)
        break;

      if (DstFile.Write(&TempBuffer[0], uiRead) == W_FAILURE)
        goto done;
    }
  }

  Res = W_SUCCESS;

done:

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#else
  const WTime tdiff = WTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sSource;
  e.m_sFile2 = sDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if W_ENABLED(W_SUPPORTS_FILE_STATS)

WResult WOSFile::GetFileStats(WStringView sFileOrFolder, WFileStats& out_stats)
{
#  if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#  endif

  WStringBuilder s = sFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  W_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const WResult Res = InternalGetFileStats(s.GetData(), out_stats);

#  if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#  else
  const WTime tdiff = WTime::MakeZero();
#  endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if W_ENABLED(W_SUPPORTS_CASE_INSENSITIVE_PATHS) && W_ENABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
WResult WOSFile::GetFileCasing(WStringView sFileOrFolder, WStringBuilder& out_sCorrectSpelling)
{
  /// \todo We should implement this also on WFileSystem, to be able to support stats through virtual filesystems

#    if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime t0 = WTime::Now();
#    endif

  WStringBuilder s(sFileOrFolder);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  W_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  WStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  out_sCorrectSpelling.Clear();

  WResult Res = W_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!WPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    if (!sCurPath.IsEmpty())
    {
      WFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == W_FAILURE)
      {
        Res = W_FAILURE;
        break;
      }

      out_sCorrectSpelling.AppendPath(stats.m_sName);
    }
    sCurPath.Append(it.GetCharacter());
    ++it;
  }

#    if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WTime tdiff = WTime::Now() - t0;
#    else
  const WTime tdiff = WTime::MakeZero();
#    endif

  EventData e;
  e.m_bSuccess = Res == W_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // W_SUPPORTS_CASE_INSENSITIVE_PATHS && W_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif   // W_SUPPORTS_FILE_STATS

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS)

void WOSFile::GatherAllItemsInFolder(WDynamicArray<WFileStats>& out_itemList, WStringView sFolder, WBitflags<WFileSystemIteratorFlags> flags /*= WFileSystemIteratorFlags::All*/)
{
  out_itemList.Clear();

  WFileSystemIterator iterator;
  iterator.StartSearch(sFolder, flags);

  if (!iterator.IsValid())
    return;

  out_itemList.Reserve(128);

  while (iterator.IsValid())
  {
    out_itemList.PushBack(iterator.GetStats());

    iterator.Next();
  }
}

WResult WOSFile::CopyFolder(WStringView sSourceFolder, WStringView sDestinationFolder, WDynamicArray<WString>* out_pFilesCopied /*= nullptr*/)
{
  WDynamicArray<WFileStats> items;
  GatherAllItemsInFolder(items, sSourceFolder);

  WStringBuilder srcPath;
  WStringBuilder dstPath;
  WStringBuilder relPath;

  for (const auto& item : items)
  {
    srcPath = item.m_sParentPath;
    srcPath.AppendPath(item.m_sName);

    relPath = srcPath;

    if (relPath.MakeRelativeTo(sSourceFolder).Failed())
      return W_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = sDestinationFolder;
    dstPath.AppendPath(relPath);

    if (item.m_bIsDirectory)
    {
      if (WOSFile::CreateDirectoryStructure(dstPath).Failed())
        return W_FAILURE;
    }
    else
    {
      if (WOSFile::CopyFile(srcPath, dstPath).Failed())
        return W_FAILURE;

      if (out_pFilesCopied)
      {
        out_pFilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return W_SUCCESS;
}

WResult WOSFile::DeleteFolder(WStringView sFolder)
{
  WDynamicArray<WFileStats> items;
  GatherAllItemsInFolder(items, sFolder);

  WStringBuilder fullPath;

  for (const auto& item : items)
  {
    if (item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (WOSFile::DeleteFile(fullPath).Failed())
      return W_FAILURE;
  }

  for (WUInt32 i = items.GetCount(); i > 0; --i)
  {
    const auto& item = items[i - 1];

    if (!item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (WOSFile::InternalDeleteDirectory(fullPath).Failed())
      return W_FAILURE;
  }

  if (WOSFile::InternalDeleteDirectory(sFolder).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

#endif // W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS)

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

void WFileSystemIterator::StartMultiFolderSearch(WArrayPtr<WString> startFolders, WStringView sSearchTerm, WBitflags<WFileSystemIteratorFlags> flags /*= WFileSystemIteratorFlags::Default*/)
{
  if (startFolders.IsEmpty())
    return;

  m_sMultiSearchTerm = sSearchTerm;
  m_Flags = flags;
  m_uiCurrentStartFolder = 0;
  m_StartFolders = startFolders;

  WStringBuilder search = startFolders[m_uiCurrentStartFolder];
  search.AppendPath(sSearchTerm);

  StartSearch(search, m_Flags);

  if (!IsValid())
  {
    Next();
  }
}

void WFileSystemIterator::Next()
{
  while (true)
  {
    const WInt32 res = InternalNext();

    if (res == 1) // success
    {
      return;
    }
    else if (res == 0) // failure
    {
      ++m_uiCurrentStartFolder;

      if (m_uiCurrentStartFolder < m_StartFolders.GetCount())
      {
        WStringBuilder search = m_StartFolders[m_uiCurrentStartFolder];
        search.AppendPath(m_sMultiSearchTerm);

        if (search.IsAbsolutePath())
        {
          StartSearch(search, m_Flags);
        }
      }
      else
      {
        return;
      }

      if (IsValid())
      {
        return;
      }
    }
    else
    {
      // call InternalNext() again
    }
  }
}

void WFileSystemIterator::SkipFolder()
{
  W_ASSERT_DEBUG(m_Flags.IsSet(WFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  W_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(WFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(WFileSystemIteratorFlags::Recursive);
}

#endif

#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

#  include <Shlobj.h>

WResult WOSFile::InternalOpen(WStringView sFile, WFileOpenMode::Enum OpenMode, WFileShareMode::Enum FileShareMode)
{
  const WTime sleepTime = WTime::MakeFromMilliseconds(20);
  WInt32 iRetries = 20;

  if (FileShareMode == WFileShareMode::Default)
  {
    // when 'default' share mode is requested, use 'share reads' when opening a file for reading
    // and use 'exclusive' when opening a file for writing

    if (OpenMode == WFileOpenMode::Read)
    {
      FileShareMode = WFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = WFileShareMode::Exclusive;
    }
  }

  DWORD dwSharedMode = 0; // exclusive access
  if (FileShareMode == WFileShareMode::SharedReads)
  {
    dwSharedMode = FILE_SHARE_READ;
  }

  while (iRetries > 0)
  {
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;

    switch (OpenMode)
    {
      case WFileOpenMode::Read:
        m_FileData.m_pFileHandle = CreateFileW(WDosDevicePath(sFile), GENERIC_READ, dwSharedMode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case WFileOpenMode::Write:
        m_FileData.m_pFileHandle = CreateFileW(WDosDevicePath(sFile), GENERIC_WRITE, dwSharedMode, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case WFileOpenMode::Append:
        m_FileData.m_pFileHandle = CreateFileW(WDosDevicePath(sFile), FILE_APPEND_DATA, dwSharedMode, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
        if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
          InternalSetFilePosition(0, WFileSeekMode::FromEnd);

        break;

        W_DEFAULT_CASE_NOT_IMPLEMENTED
    }

    const WResult res = ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? W_SUCCESS : W_FAILURE;

    if (res.Failed())
    {
      if (WOSFile::ExistsDirectory(sFile))
      {
        // trying to 'open' a directory fails with little useful error codes such as 'access denied'
        return W_FAILURE;
      }

      error = GetLastError();

      // file does not exist
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return res;
      // badly formed path, happens when two absolute paths are concatenated
      if (error == ERROR_INVALID_NAME)
        return res;

      if (error == ERROR_SHARING_VIOLATION
          // these two situations happen when the WInspector is connected
          // for some reason, the networking blocks file reading (when run on the same machine)
          // retrying fixes the problem, but can introduce very long stalls
          || error == WSAEWOULDBLOCK || error == ERROR_SUCCESS)
      {
        if (m_bRetryOnSharingViolation)
        {
          --iRetries;
          WThreadUtils::Sleep(sleepTime);
          continue; // try again
        }
        else
        {
          return res;
        }
      }

      // anything else, print an error (for now)
      WLog::Error("CreateFile failed with error {0}", WArgErrorCode(error));
    }

    return res;
  }

  return W_FAILURE;
}

void WOSFile::InternalClose()
{
  CloseHandle(m_FileData.m_pFileHandle);
  m_FileData.m_pFileHandle = INVALID_HANDLE_VALUE;
}

WResult WOSFile::InternalWrite(const void* pBuffer, WUInt64 uiBytes)
{
  const WUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBatchBytes))
      return W_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = WMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const WUInt32 uiBytes32 = static_cast<WUInt32>(uiBytes);

    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBytes32))
      return W_FAILURE;
  }

  return W_SUCCESS;
}

WUInt64 WOSFile::InternalRead(void* pBuffer, WUInt64 uiBytes)
{
  WUInt64 uiBytesRead = 0;

  const WUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;

    if (uiBytesReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = WMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const WUInt32 uiBytes32 = static_cast<WUInt32>(uiBytes);

    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;
  }

  return uiBytesRead;
}

WUInt64 WOSFile::InternalGetFilePosition() const
{
  long int uiHigh32 = 0;
  WUInt32 uiLow32 = SetFilePointer(m_FileData.m_pFileHandle, 0, &uiHigh32, FILE_CURRENT);

  return WMath::MakeUInt64(uiHigh32, uiLow32);
}

void WOSFile::InternalSetFilePosition(WInt64 iDistance, WFileSeekMode::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
    case WFileSeekMode::FromStart:
      W_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
      break;
    case WFileSeekMode::FromEnd:
      W_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
      break;
    case WFileSeekMode::FromCurrent:
      W_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
      break;
  }
}

bool WOSFile::InternalExistsFile(WStringView sFile)
{
  const DWORD dwAttrib = GetFileAttributesW(WDosDevicePath(sFile).GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

bool WOSFile::InternalExistsDirectory(WStringView sDirectory)
{
  const DWORD dwAttrib = GetFileAttributesW(WDosDevicePath(sDirectory));

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

WResult WOSFile::InternalDeleteFile(WStringView sFile)
{
  if (DeleteFileW(WDosDevicePath(sFile)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return W_SUCCESS;

    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WOSFile::InternalDeleteDirectory(WStringView sDirectory)
{
  if (RemoveDirectoryW(WDosDevicePath(sDirectory)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return W_SUCCESS;

    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WOSFile::InternalCreateDirectory(WStringView sDirectory)
{
  // handle drive letters as always successful
  if (WStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 3) // 'C:\'
    return W_SUCCESS;

  if (CreateDirectoryW(WDosDevicePath(sDirectory), nullptr) == FALSE)
  {
    const DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS)
      return W_SUCCESS;

    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WOSFile::InternalMoveFileOrDirectory(WStringView sDirectoryFrom, WStringView sDirectoryTo)
{
  if (MoveFileW(WDosDevicePath(sDirectoryFrom), WDosDevicePath(sDirectoryTo)) == 0)
  {
    return W_FAILURE;
  }
  return W_SUCCESS;
}

WString WOSFile::GetUserDataFolder(WStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDataPath = WStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  WStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

WString WOSFile::GetTempDataFolder(WStringView sSubFolder /*= nullptr*/)
{
  WStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s = WStringWChar(pPath);
      s.AppendPath("Temp");
      s_sTempDataPath = s;
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

WString WOSFile::GetUserDocumentsFolder(WStringView sSubFolder /*= {}*/)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_PublicDocuments, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDocumentsPath = WStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  WStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

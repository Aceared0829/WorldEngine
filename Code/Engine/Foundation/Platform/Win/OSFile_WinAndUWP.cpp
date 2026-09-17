#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

// Defined in Timestamp_Win.cpp
WInt64 FileTimeToEpoch(FILETIME fileTime);

WResult WOSFile::InternalGetFileStats(WStringView sFileOrFolder, WFileStats& out_Stats)
{
  WStringBuilder s = sFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  s.Trim(nullptr, "/\\");

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sParentPath.Clear();
    out_Stats.m_sName = s;
    out_Stats.m_LastModificationTime = WTimestamp::MakeInvalid();
    return W_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(WDosDevicePath(s), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return W_FAILURE;

  out_Stats.m_uiFileSize = WMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = data.cFileName;
  out_Stats.m_LastModificationTime = WTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), WSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return W_SUCCESS;
}

WStringView WOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    WUInt32 uiRequiredLength = 512;
    WTempHybridArray<wchar_t, 1024> tmp;

    while (true)
    {
      tmp.SetCountUninitialized(uiRequiredLength);

      // reset last error code
      SetLastError(ERROR_SUCCESS);

      const WUInt32 uiLength = GetModuleFileNameW(nullptr, tmp.GetData(), tmp.GetCount() - 1);
      const DWORD error = GetLastError();

      if (error == ERROR_SUCCESS)
      {
        tmp[uiLength] = L'\0';
        break;
      }

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
        uiRequiredLength += 512;
        continue;
      }

      W_REPORT_FAILURE("GetModuleFileNameW failed: {0}", WArgErrorCode(error));
    }

    // GetModuleFileNameW returns the drive letter exactly as the process was started with,
    // so launching the same executable through a lower case and an upper case path yields absolute paths
    // that differ in that one character.
    WStringBuilder sPath = WStringUtf8(tmp.GetData()).GetView();
    WPathUtils::NormalizeWindowsDriveLetter(sPath);

    s_sApplicationPath = sPath;
  }

  return s_sApplicationPath;
}

const WString WOSFile::GetCurrentWorkingDirectory()
{
  const WUInt32 uiRequiredLength = GetCurrentDirectoryW(0, nullptr);

  WTempHybridArray<wchar_t, 1024> tmp;
  tmp.SetCountUninitialized(uiRequiredLength + 16);

  if (GetCurrentDirectoryW(tmp.GetCount() - 1, tmp.GetData()) == 0)
  {
    W_REPORT_FAILURE("GetCurrentDirectoryW failed: {}", WArgErrorCode(GetLastError()));
    return WString();
  }

  tmp[uiRequiredLength] = L'\0';

  WStringBuilder clean = WStringUtf8(tmp.GetData()).GetData();
  clean.MakeCleanPath();

  return clean;
}

#endif

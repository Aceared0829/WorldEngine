#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef W_POSIX_FILE_USEOLDAPI
#  include <direct.h>
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

WResult WOSFile::InternalOpen(WStringView sFile, WFileOpenMode::Enum OpenMode, WFileShareMode::Enum FileShareMode)
{
  WStringBuilder sFileCopy = sFile;
  const char* szFile = sFileCopy;

#ifndef W_POSIX_FILE_USEOLDAPI // UWP does not support these functions
  int fd = -1;
  switch (OpenMode)
  {
    // O_CLOEXEC = don't forward to child processes
    case WFileOpenMode::Read:
      fd = open(szFile, O_RDONLY | O_CLOEXEC);
      break;
    case WFileOpenMode::Write:
    case WFileOpenMode::Append:
      fd = open(szFile, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
      break;
    default:
      break;
  }

  if (FileShareMode == WFileShareMode::Default)
  {
    if (OpenMode == WFileOpenMode::Read)
    {
      FileShareMode = WFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = WFileShareMode::Exclusive;
    }
  }

  if (fd == -1)
  {
    return W_FAILURE;
  }

  struct stat stats = {};
  if (fstat(fd, &stats) != 0)
  {
    close(fd);
    return W_FAILURE;
  }

  // Prevent opening of directories
  if ((stats.st_mode & S_IFMT) == S_IFDIR)
  {
    close(fd);
    return W_FAILURE;
  }

  const int iSharedMode = (FileShareMode == WFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;
  const WTime sleepTime = WTime::MakeFromMilliseconds(20);
  WInt32 iRetries = m_bRetryOnSharingViolation ? 20 : 1;

  while (flock(fd, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    int errorCode = errno;
    iRetries--;
    if (iRetries == 0 || errorCode != EWOULDBLOCK)
    {
      // error, could not get a lock
      close(fd);
      return W_FAILURE;
    }
    WThreadUtils::Sleep(sleepTime);
  }

  switch (OpenMode)
  {
    case WFileOpenMode::Read:
      m_FileData.m_pFileHandle = fdopen(fd, "rb");
      break;
    case WFileOpenMode::Write:
      if (ftruncate(fd, 0) < 0)
      {
        close(fd);
        return W_FAILURE;
      }
      m_FileData.m_pFileHandle = fdopen(fd, "wb");
      break;
    case WFileOpenMode::Append:
      m_FileData.m_pFileHandle = fdopen(fd, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, WFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
  {
    close(fd);
  }

#else
  W_IGNORE_UNUSED(FileShareMode);

  switch (OpenMode)
  {
    case WFileOpenMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case WFileOpenMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case WFileOpenMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, WFileSeekMode::FromEnd);

      break;
    default:
      break;
  }
#endif

  if (m_FileData.m_pFileHandle == nullptr)
  {
    return W_FAILURE;
  }

  // lock will be released automatically when the file is closed
  return W_SUCCESS;
}

void WOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

WResult WOSFile::InternalWrite(const void* pBuffer, WUInt64 uiBytes)
{
  const WUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
    {
      return W_FAILURE;
    }

    uiBytes -= uiBatchBytes;
    pBuffer = WMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const WUInt32 uiBytes32 = static_cast<WUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
    {
      return W_FAILURE;
    }
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
    const WUInt64 uiReadThisTime = fread(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle);
    uiBytesRead += uiReadThisTime;

    if (uiReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = WMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const WUInt32 uiBytes32 = static_cast<WUInt32>(uiBytes);

    uiBytesRead += fread(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle);
  }

  return uiBytesRead;
}

WUInt64 WOSFile::InternalGetFilePosition() const
{
#ifdef W_POSIX_FILE_USEOLDAPI
  return static_cast<WUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<WUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void WOSFile::InternalSetFilePosition(WInt64 iDistance, WFileSeekMode::Enum Pos) const
{
#ifdef W_POSIX_FILE_USEOLDAPI
  switch (Pos)
  {
    case WFileSeekMode::FromStart:
      W_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case WFileSeekMode::FromEnd:
      W_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case WFileSeekMode::FromCurrent:
      W_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case WFileSeekMode::FromStart:
      W_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case WFileSeekMode::FromEnd:
      W_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case WFileSeekMode::FromCurrent:
      W_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#endif
}

// this might not be defined on Windows
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

bool WOSFile::InternalExistsFile(WStringView sFile)
{
  struct stat sb;
  return (stat(WString(sFile), &sb) == 0 && !S_ISDIR(sb.st_mode));
}

bool WOSFile::InternalExistsDirectory(WStringView sDirectory)
{
  struct stat sb;
  return (stat(WString(sDirectory), &sb) == 0 && S_ISDIR(sb.st_mode));
}

WResult WOSFile::InternalDeleteFile(WStringView sFile)
{
#ifdef W_POSIX_FILE_USEWINDOWSAPI
  int iRes = _unlink(WString(sFile));
#else
  int iRes = unlink(WString(sFile));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return W_SUCCESS;

  return W_FAILURE;
}

WResult WOSFile::InternalDeleteDirectory(WStringView sDirectory)
{
#ifdef W_POSIX_FILE_USEWINDOWSAPI
  int iRes = _rmdir(WString(sDirectory));
#else
  int iRes = rmdir(WString(sDirectory));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return W_SUCCESS;

  return W_FAILURE;
}

WResult WOSFile::InternalCreateDirectory(WStringView sDirectory)
{
  // handle drive letters as always successful
  if (WStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 1) // '/'
    return W_SUCCESS;

#ifdef W_POSIX_FILE_USEWINDOWSAPI
  int iRes = _mkdir(WString(sDirectory));
#else
  int iRes = mkdir(WString(sDirectory), 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return W_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to WOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
  if (errno == EACCES && InternalExistsDirectory(sDirectory))
    return W_SUCCESS;

  return W_FAILURE;
}

WResult WOSFile::InternalMoveFileOrDirectory(WStringView sDirectoryFrom, WStringView sDirectoryTo)
{
  if (rename(WString(sDirectoryFrom), WString(sDirectoryTo)) != 0)
  {
    return W_FAILURE;
  }
  return W_SUCCESS;
}

#if W_ENABLED(W_SUPPORTS_FILE_STATS)

#  ifndef W_POSIX_FILE_NOINTERNALGETFILESTATS

WResult WOSFile::InternalGetFileStats(WStringView sFileOrFolder, WFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(WString(sFileOrFolder), &tempStat);

  if (iRes != 0)
    return W_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = WPathUtils::GetFileNameAndExtension(sFileOrFolder); // no OS support, so just pass it through
#    ifdef __USE_XOPEN2K8
  out_Stats.m_LastModificationTime = WTimestamp::MakeFromInt(tempStat.st_mtim.tv_sec * 1000000000ull + tempStat.st_mtim.tv_nsec, WSIUnitOfTime::Nanosecond);
#    else
  out_Stats.m_LastModificationTime = WTimestamp::MakeFromInt(tempStat.st_mtime, WSIUnitOfTime::Second);
#    endif
  return W_SUCCESS;
}

#  endif

#endif

#ifndef W_POSIX_FILE_NOGETAPPLICATIONPATH

WStringView WOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    char result[PATH_MAX];
    size_t length = readlink("/proc/self/exe", result, PATH_MAX);
    s_sApplicationPath = WStringView(result, result + length);
  }

  return s_sApplicationPath;
}

#endif

#ifndef W_POSIX_FILE_NOGETUSERDATAFOLDER

WString WOSFile::GetUserDataFolder(WStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    WStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath(".local", "share");
    s_sUserDataPath = sTemp;
  }

  WStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef W_POSIX_FILE_NOGETTEMPDATAFOLDER

WString WOSFile::GetTempDataFolder(WStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    WStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath(".cache");
    s_sTempDataPath = sTemp;
  }

  WStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef W_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

WString WOSFile::GetUserDocumentsFolder(WStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    WStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath("Documents");
    s_sUserDocumentsPath = sTemp;
  }

  WStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef W_POSIX_FILE_NOGETCURRENTWORKINGDIRECTORY

const WString WOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  WStringBuilder clean = getcwd(tmp, W_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#endif

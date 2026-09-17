#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

struct WMemoryMappedFileImpl
{
  WMemoryMappedFile::Mode m_Mode = WMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  WUInt64 m_uiFileSize = 0;
  int m_hFile = -1;
  WString m_sSharedMemoryName;

  ~WMemoryMappedFileImpl()
  {
#if W_ENABLED(W_SUPPORTS_MEMORY_MAPPED_FILE)
    if (m_pMappedFilePtr != nullptr)
    {
      munmap(m_pMappedFilePtr, m_uiFileSize);
      m_pMappedFilePtr = nullptr;
    }
    if (m_hFile != -1)
    {
      close(m_hFile);
      m_hFile = -1;
    }
#  ifdef W_POSIX_MMAP_SKIPUNLINK
    // shm_open / shm_unlink deprecated.
    // There is an alternative in ASharedMemory_create but that is only
    // available in API 26 upwards.
#  else
    if (!m_sSharedMemoryName.IsEmpty())
    {
      shm_unlink(m_sSharedMemoryName);
      m_sSharedMemoryName.Clear();
    }
#  endif
    m_uiFileSize = 0;
#endif
  }
};

WMemoryMappedFile::WMemoryMappedFile()
{
  m_pImpl = W_DEFAULT_NEW(WMemoryMappedFileImpl);
}

WMemoryMappedFile::~WMemoryMappedFile()
{
  Close();
}

#if W_ENABLED(W_SUPPORTS_MEMORY_MAPPED_FILE)
WResult WMemoryMappedFile::Open(WStringView sAbsolutePath, Mode mode)
{
  W_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  W_ASSERT_DEV(WPathUtils::IsAbsolutePath(sAbsolutePath), "WMemoryMappedFile::Open() can only be used with absolute file paths");

  W_LOG_BLOCK("MemoryMapFile", sAbsolutePath);

  const WStringBuilder sPath = sAbsolutePath;

  Close();

  m_pImpl->m_Mode = mode;

  int access = O_RDONLY;
  int prot = PROT_READ;
  int flags = MAP_PRIVATE;
  if (mode == Mode::ReadWrite)
  {
    access = O_RDWR;
    prot |= PROT_WRITE;
    flags = MAP_SHARED;
  }

#  ifdef W_POSIX_MMAP_MAP_POPULATE
  flags |= MAP_POPULATE;
#  endif

  m_pImpl->m_hFile = open(sPath, access | O_CLOEXEC, 0);
  if (m_pImpl->m_hFile == -1)
  {
    WLog::Error("Could not open file for memory mapping - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }
  struct stat sb;
  if (stat(sPath, &sb) == -1 || sb.st_size == 0)
  {
    WLog::Error("File for memory mapping is empty - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }
  m_pImpl->m_uiFileSize = sb.st_size;

  m_pImpl->m_pMappedFilePtr = mmap(nullptr, m_pImpl->m_uiFileSize, prot, flags, m_pImpl->m_hFile, 0);
  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    WLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }

  return W_SUCCESS;
}
#endif

#if W_ENABLED(W_SUPPORTS_SHARED_MEMORY)
WResult WMemoryMappedFile::OpenShared(WStringView sSharedName, WUInt64 uiSize, Mode mode)
{
  W_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  W_ASSERT_DEV(uiSize > 0, "WMemoryMappedFile::OpenShared() needs a valid file size to map");

  const WStringBuilder sName = sSharedName;

  W_LOG_BLOCK("MemoryMapFile", sName);

  Close();

  m_pImpl->m_Mode = mode;

  int prot = PROT_READ;
  int oflag = O_RDONLY;
  int flags = MAP_SHARED;

#  ifdef W_POSIX_MMAP_MAP_POPULATE
  flags |= MAP_POPULATE;
#  endif

  if (mode == Mode::ReadWrite)
  {
    oflag = O_RDWR;
    prot |= PROT_WRITE;
  }
  oflag |= O_CREAT;

  m_pImpl->m_hFile = shm_open(sName, oflag, 0666);
  if (m_pImpl->m_hFile == -1)
  {
    WLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }
  m_pImpl->m_sSharedMemoryName = sName;

  if (ftruncate(m_pImpl->m_hFile, uiSize) == -1)
  {
    WLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }
  m_pImpl->m_uiFileSize = uiSize;

  m_pImpl->m_pMappedFilePtr = mmap(nullptr, m_pImpl->m_uiFileSize, prot, flags, m_pImpl->m_hFile, 0);
  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    WLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return W_FAILURE;
  }
  return W_SUCCESS;
}
#endif

void WMemoryMappedFile::Close()
{
  m_pImpl = W_DEFAULT_NEW(WMemoryMappedFileImpl);
}

WMemoryMappedFile::Mode WMemoryMappedFile::GetMode() const
{
  return m_pImpl->m_Mode;
}

const void* WMemoryMappedFile::GetReadPointer(WUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  W_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  W_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, m_pImpl->m_uiFileSize - uiOffset);
  }
}

void* WMemoryMappedFile::GetWritePointer(WUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  W_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  W_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, m_pImpl->m_uiFileSize - uiOffset);
  }
}

WUInt64 WMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

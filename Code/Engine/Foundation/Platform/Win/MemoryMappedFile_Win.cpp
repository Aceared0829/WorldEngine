#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/MemoryMappedFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/PathUtils.h>
#  include <Foundation/Strings/StringConversion.h>

struct WMemoryMappedFileImpl
{
  WMemoryMappedFile::Mode m_Mode = WMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  WUInt64 m_uiFileSize = 0;
  HANDLE m_hFile = INVALID_HANDLE_VALUE;
  HANDLE m_hMapping = INVALID_HANDLE_VALUE;

  ~WMemoryMappedFileImpl()
  {
    if (m_pMappedFilePtr != nullptr)
    {
      UnmapViewOfFile(m_pMappedFilePtr);
      m_pMappedFilePtr = nullptr;
    }

    if (m_hMapping != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hMapping);
      m_hMapping = INVALID_HANDLE_VALUE;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE;
    }
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

WResult WMemoryMappedFile::Open(WStringView sAbsolutePath, Mode mode)
{
  W_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  W_ASSERT_DEV(WPathUtils::IsAbsolutePath(sAbsolutePath), "WMemoryMappedFile::Open() can only be used with absolute file paths");

  W_LOG_BLOCK("MemoryMapFile", sAbsolutePath);


  Close();

  m_pImpl->m_Mode = mode;

  DWORD access = GENERIC_READ;

  if (mode == Mode::ReadWrite)
  {
    access |= GENERIC_WRITE;
  }

  m_pImpl->m_hFile = CreateFileW(WDosDevicePath(sAbsolutePath), access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  DWORD errorCode = GetLastError();

  if (m_pImpl->m_hFile == nullptr || m_pImpl->m_hFile == INVALID_HANDLE_VALUE)
  {
    WLog::Error("Could not open file for memory mapping - {}", WArgErrorCode(errorCode));
    Close();
    return W_FAILURE;
  }

  if (GetFileSizeEx(m_pImpl->m_hFile, reinterpret_cast<LARGE_INTEGER*>(&m_pImpl->m_uiFileSize)) == FALSE || m_pImpl->m_uiFileSize == 0)
  {
    WLog::Error("File for memory mapping is empty");
    Close();
    return W_FAILURE;
  }

  m_pImpl->m_hMapping = CreateFileMappingW(m_pImpl->m_hFile, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    WLog::Error("Could not create memory mapping of file - {}", WArgErrorCode(errorCode));
    Close();
    return W_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    WLog::Error("Could not create memory mapping view of file - {}", WArgErrorCode(errorCode));
    Close();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WMemoryMappedFile::OpenShared(WStringView sSharedName, WUInt64 uiSize, Mode mode)
{
  W_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  W_ASSERT_DEV(uiSize > 0, "WMemoryMappedFile::OpenShared() needs a valid file size to map");

  W_LOG_BLOCK("MemoryMapFile", sSharedName);

  Close();

  m_pImpl->m_Mode = mode;

  DWORD errorCode = 0;
  DWORD sizeHigh = static_cast<DWORD>((uiSize >> 32) & 0xFFFFFFFFu);
  DWORD sizeLow = static_cast<DWORD>(uiSize & 0xFFFFFFFFu);

  m_pImpl->m_hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, sizeHigh,
    sizeLow, WStringWChar(sSharedName).GetData());

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    WLog::Error("Could not create memory mapping of file - {}", WArgErrorCode(errorCode));
    Close();
    return W_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    WLog::Error("Could not create memory mapping view of file - {}", WArgErrorCode(errorCode));
    Close();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

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
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(uiOffset));
  }
  else
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

void* WMemoryMappedFile::GetWritePointer(WUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  W_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  W_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(uiOffset));
  }
  else
  {
    return WMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

WUInt64 WMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

#endif

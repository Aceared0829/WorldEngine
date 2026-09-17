#include <Foundation/FoundationPCH.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>

struct WMemoryMappedFileImpl
{
  WMemoryMappedFile::Mode m_Mode = WMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  WUInt64 m_uiFileSize = 0;

  ~WMemoryMappedFileImpl() {}
};

WMemoryMappedFile::WMemoryMappedFile()
{
  m_pImpl = W_DEFAULT_NEW(WMemoryMappedFileImpl);
}

WMemoryMappedFile::~WMemoryMappedFile()
{
  Close();
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
  W_IGNORE_UNUSED(uiOffset);
  W_IGNORE_UNUSED(base);
  W_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  return m_pImpl->m_pMappedFilePtr;
}

void* WMemoryMappedFile::GetWritePointer(WUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  W_IGNORE_UNUSED(uiOffset);
  W_IGNORE_UNUSED(base);
  W_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_pImpl->m_pMappedFilePtr;
}

WUInt64 WMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>

// Reader implementation

WMemoryStreamReader::WMemoryStreamReader(const WMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)
{
}

WMemoryStreamReader::~WMemoryStreamReader() = default;

WUInt64 WMemoryStreamReader::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  W_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const WUInt64 uiBytes = WMath::Min<WUInt64>(uiBytesToRead, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    WUInt64 uiBytesLeft = uiBytes;

    while (uiBytesLeft > 0)
    {
      WArrayPtr<const WUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiReadPosition);

      W_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const WUInt64 toRead = WMath::Min<WUInt64>(data.GetCount(), uiBytesLeft);

      WMemoryUtils::Copy(static_cast<WUInt8*>(pReadBuffer), data.GetPtr(), static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      pReadBuffer = WMemoryUtils::AddByteOffset(pReadBuffer, static_cast<size_t>(toRead));                // Down-cast to size_t for 32-bit.

      m_uiReadPosition += toRead;
      uiBytesLeft -= toRead;
    }
  }
  else
  {
    m_uiReadPosition += uiBytes;
  }

  return uiBytes;
}

WUInt64 WMemoryStreamReader::SkipBytes(WUInt64 uiBytesToSkip)
{
  W_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const WUInt64 uiBytes = WMath::Min<WUInt64>(uiBytesToSkip, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void WMemoryStreamReader::SetReadPosition(WUInt64 uiReadPosition)
{
  W_ASSERT_RELEASE(uiReadPosition <= GetByteCount64(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

WUInt32 WMemoryStreamReader::GetByteCount32() const
{
  W_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize32();
}

WUInt64 WMemoryStreamReader::GetByteCount64() const
{
  W_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize64();
}

void WMemoryStreamReader::SetDebugSourceInformation(WStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////

// Writer implementation
WMemoryStreamWriter::WMemoryStreamWriter(WMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)

{
}

WMemoryStreamWriter::~WMemoryStreamWriter() = default;

WResult WMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEV(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  if (uiBytesToWrite == 0)
    return W_SUCCESS;

  W_ASSERT_DEBUG(pWriteBuffer != nullptr, "No valid buffer containing data given!");

  // Reserve the memory in the storage object, grow size if appending data (don't shrink)
  m_pStreamStorage->SetInternalSize(WMath::Max(m_pStreamStorage->GetStorageSize64(), m_uiWritePosition + uiBytesToWrite));

  {
    WUInt64 uiBytesLeft = uiBytesToWrite;

    while (uiBytesLeft > 0)
    {
      WArrayPtr<WUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiWritePosition);

      W_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const WUInt64 toWrite = WMath::Min<WUInt64>(data.GetCount(), uiBytesLeft);

      WMemoryUtils::Copy(data.GetPtr(), static_cast<const WUInt8*>(pWriteBuffer), static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      pWriteBuffer = WMemoryUtils::AddByteOffset(pWriteBuffer, static_cast<size_t>(toWrite));                     // Down-cast to size_t for 32-bit.

      m_uiWritePosition += toWrite;
      uiBytesLeft -= toWrite;
    }
  }

  return W_SUCCESS;
}

void WMemoryStreamWriter::SetWritePosition(WUInt64 uiWritePosition)
{
  W_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  W_ASSERT_RELEASE(uiWritePosition <= GetByteCount64(), "Write position must be between 0 and GetByteCount()!");
  m_uiWritePosition = uiWritePosition;
}

WUInt32 WMemoryStreamWriter::GetByteCount32() const
{
  W_ASSERT_DEV(m_uiWritePosition <= 0xFFFFFFFFllu, "Use GetByteCount64 instead of GetByteCount32");
  return (WUInt32)m_uiWritePosition;
}

WUInt64 WMemoryStreamWriter::GetByteCount64() const
{
  return m_uiWritePosition;
}

//////////////////////////////////////////////////////////////////////////

WMemoryStreamStorageInterface::WMemoryStreamStorageInterface() = default;
WMemoryStreamStorageInterface::~WMemoryStreamStorageInterface() = default;

void WMemoryStreamStorageInterface::ReadAll(WStreamReader& inout_stream, WUInt64 uiMaxBytes /*= 0xFFFFFFFFFFFFFFFFllu*/)
{
  Clear();
  WMemoryStreamWriter w(this);

  WUInt8 uiTemp[1024 * 8];

  while (uiMaxBytes > 0)
  {
    const WUInt64 uiToRead = WMath::Min<WUInt64>(uiMaxBytes, W_ARRAY_SIZE(uiTemp));

    const WUInt64 uiRead = inout_stream.ReadBytes(uiTemp, uiToRead);
    uiMaxBytes -= uiRead;

    w.WriteBytes(uiTemp, uiRead).IgnoreResult();

    if (uiRead < uiToRead)
      break;
  }
}

//////////////////////////////////////////////////////////////////////////


WRawMemoryStreamReader::WRawMemoryStreamReader() = default;

WRawMemoryStreamReader::WRawMemoryStreamReader(const void* pData, WUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

WRawMemoryStreamReader::~WRawMemoryStreamReader() = default;

void WRawMemoryStreamReader::Reset(const void* pData, WUInt64 uiDataSize)
{
  m_pRawMemory = static_cast<const WUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiReadPosition = 0;
}

WUInt64 WRawMemoryStreamReader::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  const WUInt64 uiBytes = WMath::Min<WUInt64>(uiBytesToRead, m_uiChunkSize - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    WMemoryUtils::Copy(static_cast<WUInt8*>(pReadBuffer), &m_pRawMemory[m_uiReadPosition], static_cast<size_t>(uiBytes));
  }

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

WUInt64 WRawMemoryStreamReader::SkipBytes(WUInt64 uiBytesToSkip)
{
  const WUInt64 uiBytes = WMath::Min<WUInt64>(uiBytesToSkip, m_uiChunkSize - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void WRawMemoryStreamReader::SetReadPosition(WUInt64 uiReadPosition)
{
  W_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

WUInt64 WRawMemoryStreamReader::GetByteCount() const
{
  return m_uiChunkSize;
}

void WRawMemoryStreamReader::SetDebugSourceInformation(WStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////


WRawMemoryStreamWriter::WRawMemoryStreamWriter() = default;

WRawMemoryStreamWriter::WRawMemoryStreamWriter(void* pData, WUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

WRawMemoryStreamWriter::~WRawMemoryStreamWriter() = default;

void WRawMemoryStreamWriter::Reset(void* pData, WUInt64 uiDataSize)
{
  W_ASSERT_DEV(pData != nullptr, "Invalid memory stream storage");

  m_pRawMemory = static_cast<WUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiWritePosition = 0;
}

WResult WRawMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  const WUInt64 uiBytes = WMath::Min<WUInt64>(uiBytesToWrite, m_uiChunkSize - m_uiWritePosition);

  WMemoryUtils::Copy(&m_pRawMemory[m_uiWritePosition], static_cast<const WUInt8*>(pWriteBuffer), static_cast<size_t>(uiBytes));

  m_uiWritePosition += uiBytes;

  if (uiBytes < uiBytesToWrite)
    return W_FAILURE;

  return W_SUCCESS;
}

WUInt64 WRawMemoryStreamWriter::GetStorageSize() const
{
  return m_uiChunkSize;
}

WUInt64 WRawMemoryStreamWriter::GetNumWrittenBytes() const
{
  return m_uiWritePosition;
}

void WRawMemoryStreamWriter::SetDebugSourceInformation(WStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WDefaultMemoryStreamStorage::WDefaultMemoryStreamStorage(WUInt32 uiInitialCapacity, WAllocator* pAllocator)
  : m_Chunks(pAllocator)
{
  Reserve(uiInitialCapacity);
}

WDefaultMemoryStreamStorage::~WDefaultMemoryStreamStorage()
{
  Clear();
}

void WDefaultMemoryStreamStorage::Reserve(WUInt64 uiBytes)
{
  if (m_Chunks.IsEmpty())
  {
    auto& chunk = m_Chunks.ExpandAndGetRef();
    chunk.m_Bytes = WByteArrayPtr(m_InplaceMemory);
    chunk.m_uiStartOffset = 0;
    m_uiCapacity = m_Chunks[0].m_Bytes.GetCount();
  }

  while (m_uiCapacity < uiBytes)
  {
    AddChunk(static_cast<WUInt32>(WMath::Min<WUInt64>(uiBytes - m_uiCapacity, WMath::MaxValue<WUInt32>())));
  }
}

WUInt64 WDefaultMemoryStreamStorage::GetStorageSize64() const
{
  return m_uiInternalSize;
}

void WDefaultMemoryStreamStorage::Clear()
{
  m_uiInternalSize = 0;
  m_uiLastByteAccessed = 0;
  m_uiLastChunkAccessed = 0;
  Compact();
}

void WDefaultMemoryStreamStorage::Compact()
{
  // skip chunk 0, because that's where our inplace storage is used
  while (m_Chunks.GetCount() > 1)
  {
    auto& chunk = m_Chunks.PeekBack();

    if (m_uiInternalSize > m_uiCapacity - chunk.m_Bytes.GetCount())
      break;

    m_uiCapacity -= chunk.m_Bytes.GetCount();

    WUInt8* pData = chunk.m_Bytes.GetPtr();
    W_DELETE_RAW_BUFFER(m_Chunks.GetAllocator(), pData);

    m_Chunks.PopBack();
  }
}

WUInt64 WDefaultMemoryStreamStorage::GetHeapMemoryUsage() const
{
  return m_Chunks.GetHeapMemoryUsage() + m_uiCapacity - m_Chunks[0].m_Bytes.GetCount();
}

WResult WDefaultMemoryStreamStorage::CopyToStream(WStreamWriter& inout_stream) const
{
  WUInt64 uiBytesLeft = m_uiInternalSize;
  WUInt64 uiReadPosition = 0;

  while (uiBytesLeft > 0)
  {
    WArrayPtr<const WUInt8> data = GetContiguousMemoryRange(uiReadPosition);

    W_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

    W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(data.GetPtr(), data.GetCount()));

    uiReadPosition += data.GetCount();
    uiBytesLeft -= data.GetCount();
  }

  return W_SUCCESS;
}

WArrayPtr<const WUInt8> WDefaultMemoryStreamStorage::GetContiguousMemoryRange(WUInt64 uiStartByte) const
{
  if (uiStartByte >= m_uiInternalSize)
    return {};

  // remember the last access (byte offset) and in which chunk that ended up, to speed up this lookup
  // if a read comes in that's not AFTER the previous one, just reset to the start

  if (uiStartByte < m_uiLastByteAccessed)
  {
    m_uiLastChunkAccessed = 0;
  }

  m_uiLastByteAccessed = uiStartByte;

  for (; m_uiLastChunkAccessed < m_Chunks.GetCount(); ++m_uiLastChunkAccessed)
  {
    const auto& chunk = m_Chunks[m_uiLastChunkAccessed];

    if (uiStartByte < chunk.m_uiStartOffset + chunk.m_Bytes.GetCount())
    {
      const WUInt64 uiStartByteRel = uiStartByte - chunk.m_uiStartOffset;    // start offset into the chunk
      const WUInt64 uiMaxLenRel = chunk.m_Bytes.GetCount() - uiStartByteRel; // max number of bytes to use from this chunk
      const WUInt64 uiMaxRangeRel = m_uiInternalSize - uiStartByte;          // the 'stored data' might be less than the capacity of the chunk

      return {chunk.m_Bytes.GetPtr() + uiStartByteRel, static_cast<WUInt32>(WMath::Min<WUInt64>(uiMaxRangeRel, uiMaxLenRel))};
    }
  }

  return {};
}

WArrayPtr<WUInt8> WDefaultMemoryStreamStorage::GetContiguousMemoryRange(WUInt64 uiStartByte)
{
  WArrayPtr<const WUInt8> constData = const_cast<const WDefaultMemoryStreamStorage*>(this)->GetContiguousMemoryRange(uiStartByte);
  return {const_cast<WUInt8*>(constData.GetPtr()), constData.GetCount()};
}

void WDefaultMemoryStreamStorage::SetInternalSize(WUInt64 uiSize)
{
  Reserve(uiSize);

  m_uiInternalSize = uiSize;
}

void WDefaultMemoryStreamStorage::AddChunk(WUInt32 uiMinimumSize)
{
  auto& chunk = m_Chunks.ExpandAndGetRef();

  WUInt32 uiSize = 0;

  if (m_Chunks.GetCount() < 4)
  {
    uiSize = 1024 * 4; // 4 KB
  }
  else if (m_Chunks.GetCount() < 8)
  {
    uiSize = 1024 * 64; // 64 KB
  }
  else if (m_Chunks.GetCount() < 16)
  {
    uiSize = 1024 * 1024 * 4; // 4 MB
  }
  else
  {
    uiSize = 1024 * 1024 * 64; // 64 MB
  }

  uiSize = WMath::Max(uiSize, uiMinimumSize);

  const auto& prevChunk = m_Chunks[m_Chunks.GetCount() - 2];

  chunk.m_Bytes = WArrayPtr<WUInt8>(W_NEW_RAW_BUFFER(m_Chunks.GetAllocator(), WUInt8, uiSize), uiSize);
  chunk.m_uiStartOffset = prevChunk.m_uiStartOffset + prevChunk.m_Bytes.GetCount();
  m_uiCapacity += chunk.m_Bytes.GetCount();
}

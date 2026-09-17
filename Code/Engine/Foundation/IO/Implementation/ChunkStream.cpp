#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/ChunkStream.h>

WChunkStreamWriter::WChunkStreamWriter(WStreamWriter& inout_stream)
  : m_Stream(inout_stream)
{
  m_bWritingFile = false;
  m_bWritingChunk = false;
}

void WChunkStreamWriter::BeginStream(WUInt16 uiVersion)
{
  W_ASSERT_DEV(!m_bWritingFile, "Already writing the file.");
  W_ASSERT_DEV(uiVersion > 0, "The version number must be larger than 0");

  m_bWritingFile = true;

  const char* szTag = "BGNCHNK2";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
  m_Stream.WriteBytes(&uiVersion, 2).IgnoreResult();
}

void WChunkStreamWriter::EndStream()
{
  W_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  W_ASSERT_DEV(!m_bWritingChunk, "A chunk is still open for writing: '{0}'", m_sChunkName);

  m_bWritingFile = false;

  const char* szTag = "END CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
}

void WChunkStreamWriter::BeginChunk(WStringView sName, WUInt32 uiVersion)
{
  W_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  W_ASSERT_DEV(!m_bWritingChunk, "A chunk is already open for writing: '{0}'", m_sChunkName);

  m_sChunkName = sName;

  const char* szTag = "NXT CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();

  m_Stream << m_sChunkName;
  m_Stream << uiVersion;

  m_bWritingChunk = true;
}


void WChunkStreamWriter::EndChunk()
{
  W_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  W_ASSERT_DEV(m_bWritingChunk, "No chunk is currently open.");

  m_bWritingChunk = false;

  const WUInt32 uiStorageSize = m_Storage.GetCount();
  m_Stream << uiStorageSize;
  /// \todo Write Chunk CRC

  for (WUInt32 i = 0; i < uiStorageSize;)
  {
    const WUInt32 uiRange = m_Storage.GetContiguousRange(i);

    W_ASSERT_DEBUG(uiRange > 0, "Invalid contiguous range");

    m_Stream.WriteBytes(&m_Storage[i], uiRange).IgnoreResult();
    i += uiRange;
  }

  m_Storage.Clear();
}

WResult WChunkStreamWriter::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEV(m_bWritingChunk, "No chunk is currently written to");

  const WUInt8* pBytes = (const WUInt8*)pWriteBuffer;

  for (WUInt64 i = 0; i < uiBytesToWrite; ++i)
    m_Storage.PushBack(pBytes[i]);

  return W_SUCCESS;
}



WChunkStreamReader::WChunkStreamReader(WStreamReader& inout_stream)
  : m_Stream(inout_stream)
{
  m_ChunkInfo.m_bValid = false;
  m_EndChunkFileMode = EndChunkFileMode::JustClose;
}

WUInt64 WChunkStreamReader::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  W_ASSERT_DEV(m_ChunkInfo.m_bValid, "No valid chunk available.");

  uiBytesToRead = WMath::Min<WUInt64>(uiBytesToRead, m_ChunkInfo.m_uiUnreadChunkBytes);
  m_ChunkInfo.m_uiUnreadChunkBytes -= (WUInt32)uiBytesToRead;

  return m_Stream.ReadBytes(pReadBuffer, uiBytesToRead);
}

WUInt16 WChunkStreamReader::BeginStream()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  WUInt16 uiVersion = 0;

  if (WStringUtils::IsEqual(szTag, "BGNCHNK2"))
  {
    m_Stream.ReadBytes(&uiVersion, 2);
  }
  else
  {
    // "BGN CHNK" is the old chunk identifier, before a version number was written
    W_ASSERT_DEV(WStringUtils::IsEqual(szTag, "BGN CHNK"), "Not a valid chunk file.");
  }

  TryReadChunkHeader();
  return uiVersion;
}

void WChunkStreamReader::EndStream()
{
  if (m_EndChunkFileMode == EndChunkFileMode::SkipToEnd)
  {
    while (m_ChunkInfo.m_bValid)
      NextChunk();
  }
}

void WChunkStreamReader::TryReadChunkHeader()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  if (WStringUtils::IsEqual(szTag, "END CHNK"))
    return;

  if (WStringUtils::IsEqual(szTag, "NXT CHNK"))
  {
    m_Stream >> m_ChunkInfo.m_sChunkName;
    m_Stream >> m_ChunkInfo.m_uiChunkVersion;
    m_Stream >> m_ChunkInfo.m_uiChunkBytes;
    m_ChunkInfo.m_uiUnreadChunkBytes = m_ChunkInfo.m_uiChunkBytes;

    m_ChunkInfo.m_bValid = true;

    return;
  }

  W_REPORT_FAILURE("Invalid chunk file, tag is '{0}'", szTag);
}

void WChunkStreamReader::NextChunk()
{
  if (!m_ChunkInfo.m_bValid)
    return;

  const WUInt64 uiToSkip = m_ChunkInfo.m_uiUnreadChunkBytes;
  const WUInt64 uiSkipped = SkipBytes(uiToSkip);
  W_VERIFY(uiSkipped == uiToSkip, "Corrupt chunk '{0}' (version {1}), tried to skip {2} bytes, could only read {3} bytes", m_ChunkInfo.m_sChunkName, m_ChunkInfo.m_uiChunkVersion, uiToSkip, uiSkipped);

  TryReadChunkHeader();
}

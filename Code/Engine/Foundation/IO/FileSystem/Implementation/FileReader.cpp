#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>

WResult WFileReader::Open(WStringView sFile, WUInt32 uiCacheSize /*= 1024 * 64*/,
  WFileShareMode::Enum fileShareMode /*= WFileShareMode::SharedReads*/, bool bAllowFileEvents /*= true*/)
{
  W_ASSERT_DEV(m_pDataDirReader == nullptr, "The file reader is already open. (File: '{0}')", sFile);

  uiCacheSize = WMath::Min<WUInt32>(uiCacheSize, 1024 * 1024 * 32);

  m_pDataDirReader = GetFileReader(sFile, fileShareMode, bAllowFileEvents);

  if (!m_pDataDirReader)
    return W_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheReadPosition = 0;
  m_uiBytesCached = 0;
  m_bEOF = false;

  return W_SUCCESS;
}

void WFileReader::Close()
{
  if (m_pDataDirReader)
    m_pDataDirReader->Close();

  m_pDataDirReader = nullptr;
  m_bEOF = true;
}

WUInt64 WFileReader::SkipBytes(WUInt64 uiBytesToSkip)
{
  W_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  WUInt64 uiSkipPosition = 0; // how much was skipped, yet

  // if any data is still in the cache, skip that first
  {
    const WUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
    const WUInt64 uiBytesSkippedFromCache = WMath::Min(uiCachedBytesLeft, uiBytesToSkip);
    uiSkipPosition += uiBytesSkippedFromCache;
    m_uiCacheReadPosition += uiBytesSkippedFromCache;
    uiBytesToSkip -= uiBytesSkippedFromCache;
  }

  // skip bytes on disk
  const WUInt64 uiBytesMeantToSkipFromDisk = uiBytesToSkip;
  const WUInt64 uiBytesSkippedFromDisk = m_pDataDirReader->Skip(uiBytesToSkip);
  uiSkipPosition += uiBytesSkippedFromDisk;
  uiBytesToSkip -= uiBytesSkippedFromDisk;

  // mark end of file if suitable
  const bool endOfCacheReached = m_uiCacheReadPosition == m_uiBytesCached;
  const bool endOfDiskDataReached = uiBytesSkippedFromDisk < uiBytesMeantToSkipFromDisk;
  if (endOfCacheReached && endOfDiskDataReached)
  {
    m_bEOF = true;
  }

  return uiSkipPosition;
}

WUInt64 WFileReader::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  W_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  WUInt64 uiBufferPosition = 0; // how much was read, yet
  WUInt8* pBuffer = (WUInt8*)pReadBuffer;

  if (uiBytesToRead > m_Cache.GetCount())
  {
    // if any data is still in the cache, use that first
    const WUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
    if (uiCachedBytesLeft > 0)
    {
      WMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(WUInt32)m_uiCacheReadPosition], (WUInt32)uiCachedBytesLeft);
      uiBufferPosition += uiCachedBytesLeft;
      m_uiCacheReadPosition += uiCachedBytesLeft;
      uiBytesToRead -= uiCachedBytesLeft;
    }

    // read remaining data from disk
    WUInt64 uiBytesReadFromDisk = 0;
    if (uiBytesToRead > 0)
    {
      uiBytesReadFromDisk = m_pDataDirReader->Read(&pBuffer[uiBufferPosition], uiBytesToRead);
      uiBufferPosition += uiBytesReadFromDisk;
    }

    // mark eof if we're already there
    if (uiBytesReadFromDisk == 0)
    {
      m_bEOF = true;
      return uiBufferPosition;
    }
  }
  else
  {
    while (uiBytesToRead > 0)
    {
      // determine the chunk size to read
      WUInt64 uiChunkSize = uiBytesToRead;

      const WUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
      if (uiCachedBytesLeft < uiBytesToRead)
      {
        uiChunkSize = uiCachedBytesLeft;
      }

      // copy data into the buffer
      // uiChunkSize can never be larger than the cache size, which is limited to 32 Bit
      if (uiChunkSize > 0)
      {
        WMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(WUInt32)m_uiCacheReadPosition], (WUInt32)uiChunkSize);

        // store how much was read and how much is still left to read
        uiBufferPosition += uiChunkSize;
        m_uiCacheReadPosition += uiChunkSize;
        uiBytesToRead -= uiChunkSize;
      }

      // if the cache is depleted, refill it
      // this will even be triggered if EXACTLY the amount of available bytes was read
      if (m_uiCacheReadPosition >= m_uiBytesCached)
      {
        m_uiBytesCached = m_pDataDirReader->Read(&m_Cache[0], m_Cache.GetCount());
        m_uiCacheReadPosition = 0;

        // if nothing else could be read from the file, return the number of bytes that have been read
        if (m_uiBytesCached == 0)
        {
          // if absolutely nothing could be read, we reached the end of the file (and we actually returned everything else,
          // so the file was really read to the end).
          m_bEOF = true;
          return uiBufferPosition;
        }
      }
    }
  }

  // return how much was read
  return uiBufferPosition;
}

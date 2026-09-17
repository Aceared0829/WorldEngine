#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>

WResult WFileWriter::Open(WStringView sFile, WUInt32 uiCacheSize /*= 1024 * 1024*/, WFileShareMode::Enum fileShareMode /*= WFileShareMode::Exclusive*/, bool bAllowFileEvents /*= true*/)
{
  uiCacheSize = WMath::Clamp<WUInt32>(uiCacheSize, 1024, 1024 * 1024 * 32);

  m_pDataDirWriter = GetFileWriter(sFile, fileShareMode, bAllowFileEvents);

  if (!m_pDataDirWriter)
    return W_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheWritePosition = 0;

  return W_SUCCESS;
}

void WFileWriter::Close()
{
  if (!m_pDataDirWriter)
    return;

  Flush().IgnoreResult();

  m_pDataDirWriter->Close();
  m_pDataDirWriter = nullptr;
}

WResult WFileWriter::Flush()
{
  const WResult res = m_pDataDirWriter->Write(&m_Cache[0], m_uiCacheWritePosition);
  m_uiCacheWritePosition = 0;

  return res;
}

WResult WFileWriter::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEV(m_pDataDirWriter != nullptr, "The file has not been opened (successfully).");

  if (uiBytesToWrite > m_Cache.GetCount())
  {
    // if there is more incoming data than what our cache can hold, there is no point in storing a copy
    // instead we can just pass the entire data through right away

    if (m_uiCacheWritePosition > 0)
    {
      W_SUCCEED_OR_RETURN(Flush());
    }

    return m_pDataDirWriter->Write(pWriteBuffer, uiBytesToWrite);
  }
  else
  {
    WUInt8* pBuffer = (WUInt8*)pWriteBuffer;

    while (uiBytesToWrite > 0)
    {
      // determine chunk size to be written
      WUInt64 uiChunkSize = uiBytesToWrite;

      const WUInt64 uiRemainingCache = m_Cache.GetCount() - m_uiCacheWritePosition;

      if (uiRemainingCache < uiBytesToWrite)
        uiChunkSize = uiRemainingCache;

      // copy memory
      WMemoryUtils::Copy(&m_Cache[(WUInt32)m_uiCacheWritePosition], pBuffer, (WUInt32)uiChunkSize);

      pBuffer += uiChunkSize;
      m_uiCacheWritePosition += uiChunkSize;
      uiBytesToWrite -= uiChunkSize;

      // if the cache is full or nearly full, flush it to disk
      if (m_uiCacheWritePosition + 32 >= m_Cache.GetCount())
      {
        if (Flush() == W_FAILURE)
          return W_FAILURE;
      }
    }

    return W_SUCCESS;
  }
}

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

#  include <Foundation/System/SystemInformation.h>
#  include <zstd/zstd.h>

WCompressedStreamReaderZstd::WCompressedStreamReaderZstd() = default;

WCompressedStreamReaderZstd::WCompressedStreamReaderZstd(WStreamReader* pInputStream)
{
  SetInputStream(pInputStream);
}

WCompressedStreamReaderZstd::~WCompressedStreamReaderZstd()
{
  if (m_pZstdDStream != nullptr)
  {
    ZSTD_freeDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
    m_pZstdDStream = nullptr;
  }
}

void WCompressedStreamReaderZstd::SetInputStream(WStreamReader* pInputStream)
{
  m_InBuffer.pos = 0;
  m_InBuffer.size = 0;
  m_bReachedEnd = false;
  m_pInputStream = pInputStream;

  if (m_pZstdDStream == nullptr)
  {
    m_pZstdDStream = ZSTD_createDStream();
  }

  ZSTD_initDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
}

WUInt64 WCompressedStreamReaderZstd::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  W_ASSERT_DEV(m_pInputStream != nullptr, "No input stream has been specified");

  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // Implement the 'skip n bytes' feature with a temp cache
  if (pReadBuffer == nullptr)
  {
    WUInt64 uiBytesRead = 0;
    WUInt8 uiTemp[1024];

    while (uiBytesToRead > 0)
    {
      const WUInt32 uiToRead = WMath::Min<WUInt32>(static_cast<WUInt32>(uiBytesToRead), 1024);

      const WUInt64 uiGotBytes = ReadBytes(uiTemp, uiToRead);

      uiBytesRead += uiGotBytes;
      uiBytesToRead -= uiGotBytes;

      if (uiGotBytes == 0) // prevent an endless loop
        break;
    }

    return uiBytesRead;
  }

  ZSTD_outBuffer outBuffer;
  outBuffer.dst = pReadBuffer;
  outBuffer.pos = 0;
  outBuffer.size = WMath::SafeConvertToSizeT(uiBytesToRead);

  while (outBuffer.pos < outBuffer.size)
  {
    if (RefillReadCache().Failed())
      return outBuffer.pos;

    const size_t res = ZSTD_decompressStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream), &outBuffer, reinterpret_cast<ZSTD_inBuffer*>(&m_InBuffer));
    W_IGNORE_UNUSED(res);
    W_ASSERT_DEV(!ZSTD_isError(res), "Decompressing the stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  if (m_InBuffer.pos == m_InBuffer.size)
  {
    // if we have reached the end, we have not yet read the zero-terminator
    // do this now, so that data that comes after the compressed stream can be read properly

    RefillReadCache().IgnoreResult();
  }

  return outBuffer.pos;
}

WResult WCompressedStreamReaderZstd::RefillReadCache()
{
  // if our input buffer is empty, we need to read more into our cache
  if (m_InBuffer.pos == m_InBuffer.size)
  {
    WUInt16 uiCompressedSize = 0;
    W_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(WUInt16)) == sizeof(WUInt16), "Reading the compressed chunk size from the input stream failed.");

    m_InBuffer.pos = 0;
    m_InBuffer.size = uiCompressedSize;

    if (uiCompressedSize > 0)
    {
      if (m_CompressedCache.GetCount() < uiCompressedSize)
      {
        m_CompressedCache.SetCountUninitialized(WMath::RoundUp(uiCompressedSize, 1024));

        m_InBuffer.src = m_CompressedCache.GetData();
      }

      W_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(WUInt8) * uiCompressedSize) == sizeof(WUInt8) * uiCompressedSize, "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
    }
  }

  // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
  if (m_InBuffer.size == 0)
  {
    // in this case there is also no output that can be generated anymore
    m_bReachedEnd = true;
    return W_FAILURE;
  }

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCompressedStreamWriterZstd::WCompressedStreamWriterZstd() = default;

WCompressedStreamWriterZstd::WCompressedStreamWriterZstd(WStreamWriter* pOutputStream, WUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, WUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  SetOutputStream(pOutputStream, uiMaxNumWorkerThreads, ratio, uiCompressionCacheSizeKB);
}

WCompressedStreamWriterZstd::~WCompressedStreamWriterZstd()
{
  if (m_pOutputStream != nullptr)
  {
    // NOTE: FinishCompressedStream() WILL write a couple of bytes, even if the user did not write anything.
    // If WCompressedStreamWriterZstd was not supposed to be used, this may end up in a corrupted output file.
    // W_ASSERT_DEV(m_uiWrittenBytes > 0, "Output stream was set, but not a single byte was written to the compressed stream before destruction.
    // Incorrect usage?");

    FinishCompressedStream().IgnoreResult();
  }

  if (m_pZstdCStream)
  {
    ZSTD_freeCStream(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream));
    m_pZstdCStream = nullptr;
  }
}

void WCompressedStreamWriterZstd::SetOutputStream(WStreamWriter* pOutputStream, WUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, WUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  if (m_pOutputStream == pOutputStream)
    return;

  // limit the cache to 63KB, because at 64KB we run into an endless loop due to a 16 bit overflow
  uiCompressionCacheSizeKB = WMath::Min(uiCompressionCacheSizeKB, 63u);

  // finish anything done on a previous output stream
  FinishCompressedStream().IgnoreResult();

  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;
  m_uiWrittenBytes = 0;

  if (pOutputStream != nullptr)
  {
    m_pOutputStream = pOutputStream;

    if (m_pZstdCStream == nullptr)
    {
      m_pZstdCStream = ZSTD_createCStream();
    }

    WUInt32 uiMaxCoreCount = (uiMaxNumWorkerThreads > 0) ? WMath::Clamp(WSystemInformation::Get().GetCPUCoreCount(), 1u, uiMaxNumWorkerThreads) : 0u;

    ZSTD_CCtx_reset(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_reset_session_only);
    ZSTD_CCtx_refCDict(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), nullptr);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_compressionLevel, (int)ratio);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_nbWorkers, uiMaxCoreCount);

    m_CompressedCache.SetCountUninitialized(WMath::Max(1U, uiCompressionCacheSizeKB) * 1024);

    m_OutBuffer.dst = m_CompressedCache.GetData();
    m_OutBuffer.pos = 0;
    m_OutBuffer.size = m_CompressedCache.GetCount();
  }
}

WResult WCompressedStreamWriterZstd::FinishCompressedStream()
{
  if (m_pOutputStream == nullptr)
    return W_SUCCESS;

  if (Flush().Failed())
    return W_FAILURE;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src = nullptr;

  const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_end);
  W_VERIFY(!ZSTD_isError(res), "Deinitializing the zstd compression stream failed: '{0}'", ZSTD_getErrorName(res));

  // one more flush to write out the last chunk
  if (FlushWriteCache() == W_FAILURE)
    return W_FAILURE;

  // write a zero-terminator
  const WUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(WUInt16)) == W_FAILURE)
    return W_FAILURE;

  m_uiWrittenBytes += sizeof(WUInt16);
  m_pOutputStream = nullptr;

  return W_SUCCESS;
}

WResult WCompressedStreamWriterZstd::Flush()
{
  if (m_pOutputStream == nullptr)
    return W_SUCCESS;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src = nullptr;

  while (ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_flush) > 0)
  {
    if (FlushWriteCache() == W_FAILURE)
      return W_FAILURE;
  }

  if (FlushWriteCache() == W_FAILURE)
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WCompressedStreamWriterZstd::FlushWriteCache()
{
  if (m_pOutputStream == nullptr)
    return W_SUCCESS;

  const WUInt16 uiUsedCache = static_cast<WUInt16>(m_OutBuffer.pos);

  if (uiUsedCache == 0)
    return W_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(WUInt16)) == W_FAILURE)
    return W_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(WUInt8) * uiUsedCache) == W_FAILURE)
    return W_FAILURE;

  m_uiCompressedSize += uiUsedCache;
  m_uiWrittenBytes += sizeof(WUInt16) + uiUsedCache;

  // reset the write position
  m_OutBuffer.pos = 0;

  return W_SUCCESS;
}

WResult WCompressedStreamWriterZstd::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEV(m_pZstdCStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += static_cast<WUInt32>(uiBytesToWrite);

  ZSTD_inBuffer inBuffer;
  inBuffer.pos = 0;
  inBuffer.src = pWriteBuffer;
  inBuffer.size = static_cast<size_t>(uiBytesToWrite);

  while (inBuffer.pos < inBuffer.size)
  {
    if (m_OutBuffer.pos == m_OutBuffer.size)
    {
      if (FlushWriteCache() == W_FAILURE)
        return W_FAILURE;
    }

    const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &inBuffer, ZSTD_e_continue);

    W_VERIFY(!ZSTD_isError(res), "Compressing the zstd stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  return W_SUCCESS;
}

#endif

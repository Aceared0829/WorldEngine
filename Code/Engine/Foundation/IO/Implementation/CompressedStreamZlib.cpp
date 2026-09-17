#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/Math/Math.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

#  include <zlib/zlib.h>

static voidpf zLibAlloc OF((voidpf opaque, uInt items, uInt size))
{
  W_IGNORE_UNUSED(opaque);
  return W_DEFAULT_NEW_RAW_BUFFER(WUInt8, WMath::SafeConvertToSizeT(WMath::SafeMultiply64(items, size)));
}

static void zLibFree OF((voidpf opaque, voidpf address))
{
  W_IGNORE_UNUSED(opaque);
  WUInt8* pData = (WUInt8*)address;
  W_DEFAULT_DELETE_RAW_BUFFER(pData);
}

W_DEFINE_AS_POD_TYPE(z_stream_s);

WCompressedStreamReaderZip::WCompressedStreamReaderZip() = default;

WCompressedStreamReaderZip::~WCompressedStreamReaderZip()
{
  W_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);
  W_DEFAULT_DELETE(m_pZLibStream);
}

void WCompressedStreamReaderZip::SetInputStream(WStreamReader* pInputStream, WUInt64 uiInputSize)
{
  if (m_pZLibStream)
  {
    W_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);
    W_DEFAULT_DELETE(m_pZLibStream);
  }

  m_CompressedCache.SetCountUninitialized(1024 * 4);
  m_bReachedEnd = false;
  m_pInputStream = pInputStream;
  m_uiRemainingInputSize = uiInputSize;

  {
    m_pZLibStream = W_DEFAULT_NEW(z_stream_s);
    WMemoryUtils::ZeroFill(m_pZLibStream, 1);

    m_pZLibStream->opaque = nullptr;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree = zLibFree;

    W_VERIFY(inflateInit2(m_pZLibStream, -MAX_WBITS) == Z_OK, "Initializing the zip stream for decompression failed: '{0}'", m_pZLibStream->msg);
  }
}

WUInt64 WCompressedStreamReaderZip::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
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


  m_pZLibStream->next_out = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<WUInt32>(uiBytesToRead);
  m_pZLibStream->total_out = 0;

  while (m_pZLibStream->avail_out > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0 && m_uiRemainingInputSize > 0)
    {
      WUInt64 uiReadAmount = m_CompressedCache.GetCount();
      if (m_uiRemainingInputSize < uiReadAmount)
      {
        uiReadAmount = m_uiRemainingInputSize;
      }
      if (uiReadAmount == 0)
      {
        m_bReachedEnd = true;
        return m_pZLibStream->total_out;
      }

      W_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(WUInt8) * uiReadAmount) == sizeof(WUInt8) * uiReadAmount, "Reading the compressed chunk of size {0} from the input stream failed.", uiReadAmount);
      m_pZLibStream->avail_in = static_cast<uInt>(uiReadAmount);
      m_pZLibStream->next_in = m_CompressedCache.GetData();
      m_uiRemainingInputSize -= uiReadAmount;
    }

    const int iRet = inflate(m_pZLibStream, Z_SYNC_FLUSH);
    W_ASSERT_DEV(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '{0}'", m_pZLibStream->msg);

    if (iRet == Z_STREAM_END)
    {
      m_bReachedEnd = true;
      W_ASSERT_DEV(m_pZLibStream->avail_in == 0, "The input buffer should be depleted, but {0} bytes are still there.", m_pZLibStream->avail_in);
      return m_pZLibStream->total_out;
    }
  }

  return m_pZLibStream->total_out;
}


//////////////////////////////////////////////////////////////////////////


WCompressedStreamReaderZlib::WCompressedStreamReaderZlib(WStreamReader* pInputStream)
  : m_pInputStream(pInputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);
}

WCompressedStreamReaderZlib::~WCompressedStreamReaderZlib()
{
  W_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);

  W_DEFAULT_DELETE(m_pZLibStream);
}

WUInt64 WCompressedStreamReaderZlib::ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
{
  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // if we have not read from the stream before, initialize everything
  if (m_pZLibStream == nullptr)
  {
    m_pZLibStream = W_DEFAULT_NEW(z_stream_s);
    WMemoryUtils::ZeroFill(m_pZLibStream, 1);

    m_pZLibStream->opaque = nullptr;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree = zLibFree;

    W_VERIFY(inflateInit(m_pZLibStream) == Z_OK, "Initializing the zlib stream for decompression failed: '{0}'", m_pZLibStream->msg);
  }

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


  m_pZLibStream->next_out = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<WUInt32>(uiBytesToRead);
  m_pZLibStream->total_out = 0;

  while (m_pZLibStream->avail_out > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0)
    {
      WUInt16 uiCompressedSize = 0;
      W_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(WUInt16)) == sizeof(WUInt16), "Reading the compressed chunk size from the input stream failed.");

      m_pZLibStream->avail_in = uiCompressedSize;
      m_pZLibStream->next_in = m_CompressedCache.GetData();

      if (uiCompressedSize > 0)
      {
        W_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(WUInt8) * uiCompressedSize) == sizeof(WUInt8) * uiCompressedSize, "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
      }
    }

    // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
    if (m_pZLibStream->avail_in == 0)
    {
      // in this case there is also no output that can be generated anymore
      m_bReachedEnd = true;
      return m_pZLibStream->total_out;
    }

    const int iRet = inflate(m_pZLibStream, Z_NO_FLUSH);
    W_ASSERT_DEV(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '{0}'", m_pZLibStream->msg);

    if (iRet == Z_STREAM_END)
    {
      m_bReachedEnd = true;

      // if we have reached the end, we have not yet read the zero-terminator
      // do this now, so that data that comes after the compressed stream can be read properly

      WUInt16 uiTerminator = 0;
      W_VERIFY(m_pInputStream->ReadBytes(&uiTerminator, sizeof(WUInt16)) == sizeof(WUInt16), "Reading the compressed stream terminator failed.");

      W_ASSERT_DEV(uiTerminator == 0, "Unexpected Stream Terminator: {0}", uiTerminator);
      W_ASSERT_DEV(m_pZLibStream->avail_in == 0, "The input buffer should be depleted, but {0} bytes are still there.", m_pZLibStream->avail_in);
      return m_pZLibStream->total_out;
    }
  }

  return m_pZLibStream->total_out;
}


WCompressedStreamWriterZlib::WCompressedStreamWriterZlib(WStreamWriter* pOutputStream, Compression ratio)
  : m_pOutputStream(pOutputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);

  m_pZLibStream = W_DEFAULT_NEW(z_stream_s);

  WMemoryUtils::ZeroFill(m_pZLibStream, 1);

  m_pZLibStream->opaque = nullptr;
  m_pZLibStream->zalloc = zLibAlloc;
  m_pZLibStream->zfree = zLibFree;
  m_pZLibStream->next_out = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();
  m_pZLibStream->total_out = 0;

  W_VERIFY(deflateInit(m_pZLibStream, ratio) == Z_OK, "Initializing the zlib stream for compression failed: '{0}'", m_pZLibStream->msg);
}

WCompressedStreamWriterZlib::~WCompressedStreamWriterZlib()
{
  CloseStream().IgnoreResult();
}

WResult WCompressedStreamWriterZlib::CloseStream()
{
  if (m_pZLibStream == nullptr)
    return W_SUCCESS;

  WInt32 iRes = Z_OK;
  while (iRes == Z_OK)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == W_FAILURE)
        return W_FAILURE;
    }

    iRes = deflate(m_pZLibStream, Z_FINISH);
    W_ASSERT_DEV(iRes == Z_STREAM_END || iRes == Z_OK, "Finishing the stream failed: '{0}'", m_pZLibStream->msg);
  }

  // one more flush to write out the last chunk
  if (Flush() == W_FAILURE)
    return W_FAILURE;

  // write a zero-terminator
  const WUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(WUInt16)) == W_FAILURE)
    return W_FAILURE;

  W_VERIFY(deflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib compression stream failed: '{0}'", m_pZLibStream->msg);
  W_DEFAULT_DELETE(m_pZLibStream);

  return W_SUCCESS;
}

WResult WCompressedStreamWriterZlib::Flush()
{
  if (m_pZLibStream == nullptr)
    return W_SUCCESS;

  const WUInt16 uiUsedCache = static_cast<WUInt16>(m_pZLibStream->total_out);

  if (uiUsedCache == 0)
    return W_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(WUInt16)) == W_FAILURE)
    return W_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(WUInt8) * uiUsedCache) == W_FAILURE)
    return W_FAILURE;

  m_uiCompressedSize += uiUsedCache;

  m_pZLibStream->total_out = 0;
  m_pZLibStream->next_out = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();

  return W_SUCCESS;
}

WResult WCompressedStreamWriterZlib::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEV(m_pZLibStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += uiBytesToWrite;

  m_pZLibStream->next_in = static_cast<Bytef*>(const_cast<void*>(pWriteBuffer)); // C libraries suck at type safety
  m_pZLibStream->avail_in = static_cast<WUInt32>(uiBytesToWrite);
  m_pZLibStream->total_in = 0;

  while (m_pZLibStream->avail_in > 0)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == W_FAILURE)
        return W_FAILURE;
    }

    W_VERIFY(deflate(m_pZLibStream, Z_NO_FLUSH) == Z_OK, "Compressing the zlib stream failed: '{0}'", m_pZLibStream->msg);
  }

  return W_SUCCESS;
}

#endif // BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

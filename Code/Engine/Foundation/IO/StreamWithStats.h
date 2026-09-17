
#pragma once

#include <Foundation/IO/Stream.h>

/// A stream reader that wraps another stream to track how many bytes are read from it.
class W_FOUNDATION_DLL WStreamReaderWithStats : public WStreamReader
{
public:
  WStreamReaderWithStats() = default;
  WStreamReaderWithStats(WStreamReader* pStream)
    : m_pStream(pStream)
  {
  }

  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override
  {
    const WUInt64 uiRead = m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
    m_uiBytesRead += uiRead;
    return uiRead;
  }

  WUInt64 SkipBytes(WUInt64 uiBytesToSkip) override
  {
    const WUInt64 uiSkipped = m_pStream->SkipBytes(uiBytesToSkip);
    m_uiBytesSkipped += uiSkipped;
    return uiSkipped;
  }

  /// the stream to forward all requests to
  WStreamReader* m_pStream = nullptr;

  /// the number of bytes that were read from the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  WUInt64 m_uiBytesRead = 0;

  /// the number of bytes that were skipped from the wrapped stream
  WUInt64 m_uiBytesSkipped = 0;
};

/// A stream writer that wraps another stream to track how many bytes are written to it.
class W_FOUNDATION_DLL WStreamWriterWithStats : public WStreamWriter
{
public:
  WStreamWriterWithStats() = default;
  WStreamWriterWithStats(WStreamWriter* pStream)
    : m_pStream(pStream)
  {
  }

  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override
  {
    m_uiBytesWritten += uiBytesToWrite;
    return m_pStream->WriteBytes(pWriteBuffer, uiBytesToWrite);
  }

  WResult Flush() override
  {
    return m_pStream->Flush();
  }

  /// the stream to forward all requests to
  WStreamWriter* m_pStream = nullptr;

  /// the number of bytes that were written to the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  WUInt64 m_uiBytesWritten = 0;
};

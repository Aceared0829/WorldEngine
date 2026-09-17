#pragma once

#include <Foundation/IO/OSFile.h>

class StreamComparer : public WStreamWriter
{
public:
  StreamComparer(const char* szExpectedData, bool bOnlyWriteResult = false)
  {
    m_bOnlyWriteResult = bOnlyWriteResult;
    m_szExpectedData = szExpectedData;
  }

  ~StreamComparer()
  {
    if (m_bOnlyWriteResult)
    {
      WOSFile f;
      f.Open("C:\\Code\\JSON.txt", WFileOpenMode::Write).IgnoreResult();
      f.Write(m_sResult.GetData(), m_sResult.GetElementCount()).IgnoreResult();
      f.Close();
    }
    else
      W_TEST_BOOL(*m_szExpectedData == '\0');
  }

  WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
  {
    if (m_bOnlyWriteResult)
      m_sResult.Append((const char*)pWriteBuffer);
    else
    {
      const char* szWritten = (const char*)pWriteBuffer;

      W_TEST_BOOL(WMemoryUtils::IsEqual(szWritten, m_szExpectedData, (WUInt32)uiBytesToWrite));
      m_szExpectedData += uiBytesToWrite;
    }

    return W_SUCCESS;
  }

private:
  bool m_bOnlyWriteResult;
  WStringBuilder m_sResult;
  const char* m_szExpectedData;
};


class StringStream : public WStreamReader
{
public:
  StringStream(const void* pData)
  {
    m_pData = pData;
    m_uiLength = WStringUtils::GetStringElementCount((const char*)pData);
  }

  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
  {
    uiBytesToRead = WMath::Min(uiBytesToRead, m_uiLength);
    m_uiLength -= uiBytesToRead;

    if (uiBytesToRead > 0)
    {
      WMemoryUtils::Copy((WUInt8*)pReadBuffer, (WUInt8*)m_pData, (size_t)uiBytesToRead);
      m_pData = WMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
    }

    return uiBytesToRead;
  }

private:
  const void* m_pData;
  WUInt64 m_uiLength;
};

#pragma once

#include <Foundation/IO/Stream.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Jolt.h>

/// Adapts WStreamReader to JPH::StreamIn.
class WJoltStreamIn : public JPH::StreamIn
{
public:
  explicit WJoltStreamIn(WStreamReader* pReader)
    : m_pReader(pReader)
  {
  }

  virtual void ReadBytes(void* pData, size_t uiNumBytes) override
  {
    if (m_pReader->ReadBytes(pData, uiNumBytes) < uiNumBytes)
      m_bEOF = true;
  }

  virtual bool IsEOF() const override { return m_bEOF; }
  virtual bool IsFailed() const override { return false; }

private:
  WStreamReader* m_pReader = nullptr;
  bool m_bEOF = false;
};

/// Adapts WStreamWriter to JPH::StreamOut.
class WJoltStreamOut : public JPH::StreamOut
{
public:
  explicit WJoltStreamOut(WStreamWriter* pWriter)
    : m_pWriter(pWriter)
  {
  }

  virtual void WriteBytes(const void* pData, size_t uiNumBytes) override
  {
    if (m_pWriter->WriteBytes(pData, uiNumBytes).Failed())
      m_bFailed = true;
  }

  virtual bool IsFailed() const override { return m_bFailed; }

private:
  WStreamWriter* m_pWriter = nullptr;
  bool m_bFailed = false;
};

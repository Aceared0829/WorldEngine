#pragma once

#include <Foundation/IO/MemoryStream.h>

class W_FOUNDATION_DLL WTelemetryMessage
{
public:
  WTelemetryMessage();
  WTelemetryMessage(const WTelemetryMessage& rhs);
  ~WTelemetryMessage();

  void operator=(const WTelemetryMessage& rhs);

  W_ALWAYS_INLINE WStreamReader& GetReader() { return m_Reader; }
  W_ALWAYS_INLINE WStreamWriter& GetWriter() { return m_Writer; }

  W_ALWAYS_INLINE WUInt32 GetSystemID() const { return m_uiSystemID; }
  W_ALWAYS_INLINE WUInt32 GetMessageID() const { return m_uiMsgID; }

  W_ALWAYS_INLINE void SetMessageID(WUInt32 uiSystemID, WUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID = uiMessageID;
  }

  // WUInt64 GetMessageSize() const { return m_Storage.GetStorageSize64(); }

private:
  friend class WTelemetry;

  WUInt32 m_uiSystemID;
  WUInt32 m_uiMsgID;

  WContiguousMemoryStreamStorage m_Storage;
  WMemoryStreamReader m_Reader;
  WMemoryStreamWriter m_Writer;
};

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/LogEntry.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WLogMsgType, 1)
  W_BITFLAGS_CONSTANTS(WLogMsgType::Flush, WLogMsgType::BeginGroup, WLogMsgType::EndGroup, WLogMsgType::None)
  W_BITFLAGS_CONSTANTS(WLogMsgType::ErrorMsg, WLogMsgType::SeriousWarningMsg, WLogMsgType::WarningMsg, WLogMsgType::SuccessMsg, WLogMsgType::InfoMsg, WLogMsgType::DevMsg, WLogMsgType::DebugMsg, WLogMsgType::All)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WLogEntry, WNoBase, 1, WRTTIDefaultAllocator<WLogEntry>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Msg", m_sMsg),
    W_MEMBER_PROPERTY("Tag", m_sTag),
    W_ENUM_MEMBER_PROPERTY("Type", WLogMsgType, m_Type),
    W_MEMBER_PROPERTY("Indentation", m_uiIndentation),
    W_MEMBER_PROPERTY("Time", m_fSeconds),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WLogEntry::WLogEntry() = default;

WLogEntry::WLogEntry(const WLoggingEventData& le)
{
  m_sMsg = le.m_sText;
  m_sTag = le.m_sTag;
  m_Type = le.m_EventType;
  m_uiIndentation = le.m_uiIndentation;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = le.m_fSeconds;
#else
  m_fSeconds = 0.0f;
#endif
}

WLogEntryDelegate::WLogEntryDelegate(Callback callback, WLogMsgType::Enum logLevel)
  : m_Callback(callback)
{
  SetLogLevel(logLevel);
}

void WLogEntryDelegate::HandleLogMessage(const WLoggingEventData& le)
{
  WLogEntry e(le);
  m_Callback(e);
}

W_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_LogEntry);

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/Log.h>

namespace WLogWriter
{
  /// This log-writer will broadcast all messages through WTelemetry, such that external applications can display the log messages.
  class Telemetry
  {
  public:
    /// Register this at WLog to broadcast all log messages through WTelemetry.
    static void LogMessageHandler(const WLoggingEventData& eventData)
    {
      WTelemetryMessage msg;
      msg.SetMessageID(' LOG', ' MSG');

      msg.GetWriter() << (WInt8)eventData.m_EventType;
      msg.GetWriter() << (WUInt8)eventData.m_uiIndentation;
      msg.GetWriter() << eventData.m_sTag;
      msg.GetWriter() << eventData.m_sText;

      if (eventData.m_EventType == WLogMsgType::EndGroup)
      {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
        msg.GetWriter() << eventData.m_fSeconds;
#else
        msg.GetWriter() << 0.0f;
#endif
      }

      WTelemetry::Broadcast(WTelemetry::Reliable, msg);
    }
  };
} // namespace WLogWriter

void AddLogWriter()
{
  WGlobalLog::AddLogWriter(&WLogWriter::Telemetry::LogMessageHandler);
}

void RemoveLogWriter()
{
  WGlobalLog::RemoveLogWriter(&WLogWriter::Telemetry::LogMessageHandler);
}



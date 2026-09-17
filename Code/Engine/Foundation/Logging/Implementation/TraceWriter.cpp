#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/Tracing/TraceProvider.h>

void WLogWriter::Tracing::LogMessageHandler(const WLoggingEventData& eventData)
{
  const WStringBuilder sTemp = eventData.m_sText;

  switch (eventData.m_EventType)
  {
    case WLogMsgType::BeginGroup:
      W_TRACE_SCOPE_BEGIN("LogBlock", WTraceLevel::Info,
        W_TRACE_VALUE("Text", sTemp.GetData()),
        W_TRACE_VALUE("Type", (int)eventData.m_EventType),
        W_TRACE_VALUE("Indentation", eventData.m_uiIndentation));
      break;
    case WLogMsgType::EndGroup:
      W_TRACE_SCOPE_END("LogBlock");
      break;
    default:
      W_TRACE_EVENT("LogMessage", WTraceLevel::Info,
        W_TRACE_VALUE("Text", sTemp.GetData()),
        W_TRACE_VALUE("Type", (int)eventData.m_EventType),
        W_TRACE_VALUE("Indentation", eventData.m_uiIndentation));
      break;
  }
}

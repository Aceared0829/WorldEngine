#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Logging/TraceWriter.h>

void WLog::Print(const char* szText)
{
  printf("%s", szText);

  WLoggingEventData data;
  data.m_EventType = WLogMsgType::InfoMsg;
  data.m_sText = szText;
  WLogWriter::Tracing::LogMessageHandler(data);

  if (s_CustomPrintFunction)
  {
    s_CustomPrintFunction(szText);
  }

  fflush(stdout);
  fflush(stderr);
}

void WLog::OsMessageBox(const WFormatString& text)
{
  WStringBuilder tmp;
  WStringBuilder display = text.GetText(tmp);
  display.Trim(" \n\r\t");

  WLog::Print(display);
  W_ASSERT_NOT_IMPLEMENTED;
}

#endif

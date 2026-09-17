#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Logging/TraceWriter.h>

void WLog::Print(const char* szText)
{
  printf("%s", szText);

  WLoggingEventData data;
  data.m_EventType = WLogMsgType::InfoMsg;
  data.m_sText = szText;
  WLogWriter::Tracing::LogMessageHandler(data);

  OutputDebugStringW(WStringWChar(szText).GetData());

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

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  const char* title = "";
  if (WApplication::GetApplicationInstance())
  {
    title = WApplication::GetApplicationInstance()->GetApplicationName();
  }

  MessageBoxW(nullptr, WStringWChar(display).GetData(), WStringWChar(title), MB_OK);
#  else
  WLog::Print(display);
  W_ASSERT_NOT_IMPLEMENTED;
#  endif
}

#endif

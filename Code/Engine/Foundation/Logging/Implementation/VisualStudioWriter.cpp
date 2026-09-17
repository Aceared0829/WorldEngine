#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>

void WLogWriter::VisualStudio::LogMessageHandler(const WLoggingEventData& eventData)
{
  if (eventData.m_EventType == WLogMsgType::Flush)
    return;

#  if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  if (eventData.m_sTag.IsEqual_NoCase("beep"))
  {
    MessageBeep(0xFFFFFFFF);
  }
#  endif

  static WMutex WriterLock; // will only be created if this writer is used at all
  W_LOCK(WriterLock);

  if (eventData.m_EventType == WLogMsgType::BeginGroup)
    OutputDebugStringA("\n");

  for (WUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugStringA(" ");

  WStringBuilder s;

  switch (eventData.m_EventType)
  {
    case WLogMsgType::BeginGroup:
      s.SetFormat("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::EndGroup:
#  if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      s.SetFormat("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.SetFormat("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::ErrorMsg:
      s.SetFormat("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::SeriousWarningMsg:
      s.SetFormat("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::WarningMsg:
      s.SetFormat("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::SuccessMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::InfoMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::DevMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    case WLogMsgType::DebugMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));
      break;

    default:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(WStringWChar(s));

      WLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void WLogWriter::VisualStudio::LogMessageHandler(const WLoggingEventData& eventData)
{
}

#endif

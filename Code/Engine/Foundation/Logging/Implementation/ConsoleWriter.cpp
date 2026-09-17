#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Time/Timestamp.h>

#include <ConsoleWriter_Platform.inl>

WLog::TimestampMode WLogWriter::Console::s_TimestampMode = WLog::TimestampMode::None;

void WLogWriter::Console::LogMessageHandler(const WLoggingEventData& eventData)
{
  WStringBuilder sTimestamp;
  WLog::GenerateFormattedTimestamp(s_TimestampMode, sTimestamp);

  static WMutex WriterLock; // will only be created if this writer is used at all
  W_LOCK(WriterLock);

  if (eventData.m_EventType == WLogMsgType::BeginGroup)
    printf("\n");

  WTempHybridArray<char, 11> indentation;
  indentation.SetCount(eventData.m_uiIndentation + 1, ' ');
  indentation[eventData.m_uiIndentation] = 0;

  WStringBuilder sTemp1, sTemp2;

  switch (eventData.m_EventType)
  {
    case WLogMsgType::Flush:
      fflush(stdout);
      break;

    case WLogMsgType::BeginGroup:
      SetConsoleColor(0x02);
      printf("%s+++++ %s (%s) +++++\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_sTag.GetData(sTemp2));
      break;

    case WLogMsgType::EndGroup:
      SetConsoleColor(0x02);
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      printf("%s----- %s (%.6f sec)-----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_fSeconds);
#else
      printf("%s----- %s (%s)-----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), "timing info not available");
#endif
      break;

    case WLogMsgType::ErrorMsg:
      SetConsoleColor(0x0C);
      printf("%s%sError: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case WLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("%s%sSeriously: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("%s%sWarning: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case WLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    default:
      SetConsoleColor(0x0D);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));

      WLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}

void WLogWriter::Console::SetTimestampMode(WLog::TimestampMode mode)
{
  s_TimestampMode = mode;
}

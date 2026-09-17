#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/TextFileWriter.h>
#include <Foundation/Time/Timestamp.h>

WLogWriter::TextFile::~TextFile()
{
  EndLog();
}

WResult WLogWriter::TextFile::BeginLog(WStringView sFile)
{
  W_LOCK(m_Mutex);

  EndLog();

  WStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  WStringBuilder sFolder = sPath;
  sFolder.PathParentDirectory();

  if (!sFolder.IsEmpty())
  {
    W_SUCCEED_OR_RETURN(WOSFile::CreateDirectoryStructure(sFolder));
  }

  return m_File.Open(sPath, WFileOpenMode::Write, WFileShareMode::SharedReads);
}

void WLogWriter::TextFile::EndLog()
{
  W_LOCK(m_Mutex);

  m_File.Close();
}

bool WLogWriter::TextFile::IsOpen() const
{
  W_LOCK(m_Mutex);

  return m_File.IsOpen();
}

void WLogWriter::TextFile::SetTimestampMode(WLog::TimestampMode mode)
{
  W_LOCK(m_Mutex);

  m_TimestampMode = mode;
}

void WLogWriter::TextFile::LogMessageHandler(const WLoggingEventData& eventData)
{
  WStringBuilder sTimestamp;
  WLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  WTempHybridArray<char, 11> indentation;
  indentation.SetCount(eventData.m_uiIndentation + 1, ' ');
  indentation[eventData.m_uiIndentation] = 0;

  WStringBuilder sText, sTemp1, sTemp2;

  switch (eventData.m_EventType)
  {
    case WLogMsgType::Flush:
      // writes are unbuffered, nothing to do
      return;

    case WLogMsgType::BeginGroup:
      sText.SetFormat("\n{0}+++++ {1} ({2}) +++++\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_sTag.GetData(sTemp2));
      break;

    case WLogMsgType::EndGroup:
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("{0}----- {1} ({2} sec) -----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), WArgF(eventData.m_fSeconds, 6));
#else
      sText.SetFormat("{0}----- {1} ({2}) -----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), "timing info not available");
#endif
      break;

    case WLogMsgType::ErrorMsg:
      sText.SetFormat("{0}{1}Error: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::SeriousWarningMsg:
      sText.SetFormat("{0}{1}Seriously: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    case WLogMsgType::WarningMsg:
      sText.SetFormat("{0}{1}Warning: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    default:
      sText.SetFormat("{0}{1}{2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;
  }

  W_LOCK(m_Mutex);

  if (!m_File.IsOpen())
    return;

  m_File.Write(sText.GetData(), sText.GetElementCount()).IgnoreResult();
}

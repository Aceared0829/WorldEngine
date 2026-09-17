#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Timestamp.h>

WLogWriter::HTML::~HTML()
{
  EndLog();
}

void WLogWriter::HTML::BeginLog(WStringView sFile, WStringView sAppTitle)
{
  const WUInt32 uiLogCache = 1024 * 10;

  WStringBuilder sNewName;
  if (m_File.Open(sFile.GetData(sNewName), uiLogCache, WFileShareMode::SharedReads) == W_FAILURE)
  {
    for (WUInt32 i = 1; i < 32; ++i)
    {
      const WStringBuilder sName = WPathUtils::GetFileName(sFile);

      sNewName.SetFormat("{0}_{1}", sName, i);

      WStringBuilder sPath = sFile;
      sPath.ChangeFileName(sNewName);

      if (m_File.Open(sPath.GetData(), uiLogCache) == W_SUCCESS)
        break;
    }
  }

  if (!m_File.IsOpen())
  {
    WLog::Error("Could not open Log-File \"{0}\".", sFile);
    return;
  }

  WStringBuilder sText;
  sText.SetFormat("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>Log - {0}</TITLE></HEAD><BODY>", sAppTitle);

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
}

void WLogWriter::HTML::EndLog()
{
  if (!m_File.IsOpen())
    return;

  WriteString("", 0);
  WriteString("", 0);
  WriteString(" <<< HTML-Log End >>> ", 0);
  WriteString("", 0);
  WriteString("", 0);

  WStringBuilder sText;
  sText.SetFormat("</BODY></HTML>");

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();

  m_File.Close();
}

const WFileWriter& WLogWriter::HTML::GetOpenedLogFile() const
{
  return m_File;
}

void WLogWriter::HTML::SetTimestampMode(WLog::TimestampMode mode)
{
  m_TimestampMode = mode;
}

void WLogWriter::HTML::LogMessageHandler(const WLoggingEventData& eventData)
{
  if (!m_File.IsOpen())
    return;

  WStringBuilder sOriginalText = eventData.m_sText;

  WStringBuilder sTag = eventData.m_sTag;

  // Cannot write <, > or & to HTML, must be escaped
  sOriginalText.ReplaceAll("&", "&amp;");
  sOriginalText.ReplaceAll("<", "&lt;");
  sOriginalText.ReplaceAll(">", "&gt;");
  sOriginalText.ReplaceAll("\n", "<br>\n");

  sTag.ReplaceAll("&", "&amp;");
  sTag.ReplaceAll("<", "&lt;");
  sTag.ReplaceAll(">", "&gt;");

  WStringBuilder sTimestamp;
  WLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  bool bFlushWriteCache = false;

  WStringBuilder sText;

  switch (eventData.m_EventType)
  {
    case WLogMsgType::Flush:
      bFlushWriteCache = true;
      break;

    case WLogMsgType::BeginGroup:
      sText.SetFormat("<br><font color=\"#8080FF\"><b> <<< <u>{0}</u> >>> </b> ({1}) </font><br><table width=100%% border=0><tr width=100%%><td "
                      "width=10></td><td width=*>\n",
        sOriginalText, sTag);
      break;

    case WLogMsgType::EndGroup:
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1} sec)>>> </b></font><br><br>\n", sOriginalText, WArgF(eventData.m_fSeconds, 4));
#else
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1})>>> </b></font><br><br>\n", sOriginalText, "timing info not available");
#endif
      break;

    case WLogMsgType::ErrorMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF0000\"><b><u>Error:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::SeriousWarningMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF4000\"><b><u>Seriously:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::WarningMsg:
      sText.SetFormat("{0}<font color=\"#FF8000\"><u>Warning:</u> {1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::SuccessMsg:
      sText.SetFormat("{0}<font color=\"#009000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::InfoMsg:
      sText.SetFormat("{0}<font color=\"#000000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::DevMsg:
      sText.SetFormat("{0}<font color=\"#3030F0\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case WLogMsgType::DebugMsg:
      sText.SetFormat("{0}<font color=\"#A000FF\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    default:
      sText.SetFormat("{0}<font color=\"#A0A0A0\">{1}</font><br>\n", sTimestamp, sOriginalText);

      WLog::Warning("Unknown Message Type {1}", eventData.m_EventType);
      break;
  }

  if (!sText.IsEmpty())
  {
    m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
  }

  if (bFlushWriteCache)
  {
    m_File.Flush().IgnoreResult();
  }
}

void WLogWriter::HTML::WriteString(WStringView sText, WUInt32 uiColor)
{
  WStringBuilder sTemp;
  sTemp.SetFormat("<font color=\"#{0}\">{1}</font>", WArgU(uiColor, 1, false, 16, true), sText);

  m_File.WriteBytes(sTemp.GetData(), sizeof(char) * sTemp.GetElementCount()).IgnoreResult();
}

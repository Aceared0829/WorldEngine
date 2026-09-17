#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>

namespace WStackTraceLogParser
{
  static void StackTraceLogCallback(const WStringView& sLogText)
  {
    WStringView sFileName;
    WInt32 lineNumber;

    if (!ParseStackTraceFileNameAndLineNumber(sLogText, sFileName, lineNumber))
    {
      if (!ParseAssertFileNameAndLineNumber(sLogText, sFileName, lineNumber))
      {
        return;
      }
    }

    WCppSettings cpp;
    cpp.Load().IgnoreResult();
    const WStatus res = WCppProject::OpenInCodeEditor(sFileName, lineNumber);
  }

  bool ParseAssertFileNameAndLineNumber(const WStringView& sLine, WStringView& ref_sFileName, WInt32& ref_iLineNumber)
  {
    const char* szFileMarker = "File: ";
    const WUInt32 fileMarkerLength = WStringUtils::GetStringElementCount(szFileMarker);
    const char* szLineMarker = "Line: ";
    const WUInt32 lineMarkerLength = WStringUtils::GetStringElementCount(szLineMarker);

    if (sLine.FindSubString("*** Assertion ***") == nullptr)
    {
      return false;
    }

    const char* szFile = sLine.FindSubString(szFileMarker);
    if (szFile == nullptr)
    {
      return false;
    }

    const char* szLine = sLine.FindSubString(szLineMarker);
    if (szLine == nullptr)
    {
      return false;
    }

    const char* szFileEnd = WStringUtils::FindSubString(szFile + fileMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szFileEnd == nullptr)
    {
      return false;
    }

    const char* szLineEnd = WStringUtils::FindSubString(szLine + lineMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szLineEnd == nullptr)
    {
      return false;
    }

    ref_sFileName = WStringView(szFile + fileMarkerLength + 1, szFileEnd);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const WResult res = WConversionUtils::StringToInt(WStringView(szLine + lineMarkerLength + 1, szLineEnd), ref_iLineNumber);
    if (res != W_SUCCESS)
    {
      return false;
    }

    return true;
  }

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  bool ParseStackTraceFileNameAndLineNumber(const WStringView& sLine, WStringView& ref_sFileName, WInt32& ref_iLineNumber)
  {
    const char* szEndFileNameMarker = "(";
    const char* szEndLineNumberMarker = "):";

    const char* szEndLineNumber = sLine.FindSubString(szEndLineNumberMarker);
    if (szEndLineNumber == nullptr)
    {
      return false;
    }

    const char* szEndFileName = sLine.FindLastSubString(szEndFileNameMarker, szEndLineNumber);
    if (szEndFileName == nullptr)
    {
      return false;
    }

    ref_sFileName = WStringView(sLine.GetStartPointer(), szEndFileName);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const WResult res = WConversionUtils::StringToInt(WStringView(szEndFileName + 1, szEndLineNumber), ref_iLineNumber);
    if (res != W_SUCCESS)
    {
      return false;
    }

    return true;
  }
#else
  bool ParseStackTraceFileNameAndLineNumber(const WStringView& sLine, WStringView& ref_sFileName, WInt32& ref_iLineNumber)
  {
    return false;
  }
#endif

  void Register()
  {
    WQtLogWidget::AddLogItemContextActionCallback("StackTraceLog", &StackTraceLogCallback);
  }

  void Unregister()
  {
    WQtLogWidget::RemoveLogItemContextActionCallback("StackTraceLog");
  }
} // namespace WStackTraceLogParser

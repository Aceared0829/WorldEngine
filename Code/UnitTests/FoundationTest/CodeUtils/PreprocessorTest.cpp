#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

W_CREATE_SIMPLE_TEST_GROUP(CodeUtils);

WResult FileLocator(WStringView sCurAbsoluteFile, WStringView sIncludeFile, WPreprocessor::IncludeType incType, WStringBuilder& out_sAbsoluteFilePath)
{
  WStringBuilder& s = out_sAbsoluteFilePath;

  if (incType == WPreprocessor::RelativeInclude)
  {
    s = sCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(sIncludeFile);
    s.MakeCleanPath();
  }
  else if (incType == WPreprocessor::GlobalInclude)
  {
    s = "Preprocessor";
    s.AppendPath(sIncludeFile);
    s.MakeCleanPath();
  }
  else
    s = sIncludeFile;

  return W_SUCCESS;
}

class Logger : public WLogInterface
{
public:
  virtual void HandleLogMessage(const WLoggingEventData& le) override { m_sOutput.AppendFormat("Log: '{0}'\r\n", le.m_sText); }

  void EventHandler(const WPreprocessor::ProcessingEvent& ed)
  {
    switch (ed.m_Type)
    {
      case WPreprocessor::ProcessingEvent::Error:
      case WPreprocessor::ProcessingEvent::Warning:
        m_EventStack.PushBack(ed);
        break;
      case WPreprocessor::ProcessingEvent::BeginExpansion:
        m_EventStack.PushBack(ed);
        return;
      case WPreprocessor::ProcessingEvent::EndExpansion:
        m_EventStack.PopBack();
        return;
      default:
        return;
    }

    for (WUInt32 i = 0; i < m_EventStack.GetCount(); ++i)
    {
      const WPreprocessor::ProcessingEvent& event = m_EventStack[i];

      if (event.m_pToken != nullptr)
        m_sOutput.AppendFormat("{0}: Line {1} [{2}]: ", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn);

      switch (event.m_Type)
      {
        case WPreprocessor::ProcessingEvent::Error:
          m_sOutput.Append("Error: ");
          break;
        case WPreprocessor::ProcessingEvent::Warning:
          m_sOutput.Append("Warning: ");
          break;
        case WPreprocessor::ProcessingEvent::BeginExpansion:
          if (event.m_pToken != nullptr)
            m_sOutput.AppendFormat("In Macro: '{0}'", WString(event.m_pToken->m_DataView));
          break;
        case WPreprocessor::ProcessingEvent::EndExpansion:
          break;

        default:
          break;
      }

      m_sOutput.AppendFormat("{0}\r\n", event.m_sInfo);
    }

    m_EventStack.PopBack();
  }

  WDeque<WPreprocessor::ProcessingEvent> m_EventStack;
  WStringBuilder m_sOutput;
};

W_CREATE_SIMPLE_TEST(CodeUtils, Preprocessor)
{
  WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
  WStringBuilder sWriteDir = WTestFramework::GetInstance()->GetAbsOutputPath();

  W_TEST_BOOL(WFileSystem::DetectSdkRootDirectory() == W_SUCCESS);
  W_TEST_BOOL(WFileSystem::AddDataDirectory(sReadDir, "PreprocessorTest") == W_SUCCESS);
  W_TEST_BOOL_MSG(WFileSystem::AddDataDirectory(sWriteDir, "PreprocessorTest", "output", WDataDirUsage::AllowWrites) == W_SUCCESS, "Failed to mount data dir '%s'", sWriteDir.GetData());

  WTokenizedFileCache SharedCache;

  /// \todo Add tests for the following:
  /*
    macro expansion
    macro expansion in #if
    custom defines from outside
    stringify invalid token
    concatenate invalid tokens, tokens that yield valid macro
    broken function macros (missing parenthesis etc.)
    errors after #line directive
    errors in
      #line directive
      #define
      #ifdef
      etc.

    Done:
    #pragma once
    #include "" and <>
    #define defined / __LINE__ / __FILE__
    #line, __LINE__, __FILE__
    stringification with comments, newlines and spaces
    concatenation (maybe more?)
    bad #include
    comments
    newlines in some weird places
    too few, too many parameters
    __VA_ARGS__
    expansion that needs several iterations
    stringification of strings and special characters (\n)
    commas in macro parameters
    #undef
    unlocateable include file
    pass through #pragma
    pass through #line
    invalid #if, #else, #elif, #endif nesting
    #ifdef, #if etc.
    mathematical expressions
    boolean expressions
    bitwise expressions
    expand to self (with, without parameters, with parameters to expand)
    incorrect expressions
    Strings with line breaks in them
    Redefining without undefining a macro
    */

  {
    struct PPTestSettings
    {
      PPTestSettings(const char* szFileName, bool bPassThroughLines = false, bool bPassThroughPragmas = false, bool bPassThroughUnknownCommands = false)
        : m_szFileName(szFileName)
        , m_bPassThroughLines(bPassThroughLines)
        , m_bPassThroughPragmas(bPassThroughPragmas)
        , m_bPassThroughUnknownCommands(bPassThroughUnknownCommands)
      {
      }

      const char* m_szFileName;
      bool m_bPassThroughLines;
      bool m_bPassThroughPragmas;
      bool m_bPassThroughUnknownCommands;
    };

    PPTestSettings TestSettings[] = {
      PPTestSettings("IncludeViaMacro"),
      PPTestSettings("PragmaOnce"),
      PPTestSettings("LinePragmaPassThrough", true, true),
      PPTestSettings("Undef"),
      PPTestSettings("InvalidIf1"),
      PPTestSettings("Parameters"),
      PPTestSettings("LineControl"),
      PPTestSettings("LineControl2"),
      PPTestSettings("DefineFile"),
      PPTestSettings("DefineLine"),
      PPTestSettings("DefineDefined"),
      PPTestSettings("Stringify"),
      PPTestSettings("BuildFlags"),
      PPTestSettings("Empty"),
      PPTestSettings("Test1"),
      PPTestSettings("FailedInclude"), /// \todo Better error message
      PPTestSettings("PassThroughUnknown", false, false, true),
      PPTestSettings("IncorrectNesting1"),
      PPTestSettings("IncorrectNesting2"),
      PPTestSettings("IncorrectNesting3"),
      PPTestSettings("IncorrectNesting4"),
      PPTestSettings("IncorrectNesting5"),
      PPTestSettings("IncorrectNesting6"),
      PPTestSettings("IncorrectNesting7"),
      PPTestSettings("Error"),
      PPTestSettings("Warning"),
      PPTestSettings("Expressions"),
      PPTestSettings("ExpressionsBit"),
      PPTestSettings("ExpandSelf"),
      PPTestSettings("InvalidLogic1"),
      PPTestSettings("InvalidLogic2"),
      PPTestSettings("InvalidLogic3"),
      PPTestSettings("InvalidLogic4"),
      PPTestSettings("InvalidExpandSelf1"),
      PPTestSettings("InvalidExpandSelf2"),
      PPTestSettings("ErrorNoQuotes"),
      PPTestSettings("ErrorBadQuotes"),
      PPTestSettings("ErrorBadQuotes2"),
      PPTestSettings("ErrorLineBreaks"),
      PPTestSettings("Redefine"),
      PPTestSettings("ErrorBadBrackets"),
      PPTestSettings("IfTrueFalse"),
      PPTestSettings("RawStrings1"),
      PPTestSettings("RawStrings2"),
    };

    WStringBuilder sOutput;
    WStringBuilder fileName;
    WStringBuilder fileNameOut;
    WStringBuilder fileNameExp;

    for (int i = 0; i < W_ARRAY_SIZE(TestSettings); i++)
    {
      W_TEST_BLOCK(WTestBlock::Enabled, TestSettings[i].m_szFileName)
      {
        Logger log;

        WPreprocessor pp;
        pp.SetLogInterface(&log);
        pp.SetPassThroughLine(TestSettings[i].m_bPassThroughLines);
        pp.SetPassThroughPragma(TestSettings[i].m_bPassThroughPragmas);
        pp.SetFileLocatorFunction(FileLocator);
        pp.SetCustomFileCache(&SharedCache);
        pp.m_ProcessingEvents.AddEventHandler(WDelegate<void(const WPreprocessor::ProcessingEvent&)>(&Logger::EventHandler, &log));
        pp.AddCustomDefine("PP_OBJ").IgnoreResult();
        pp.AddCustomDefine("PP_FUNC(a) a").IgnoreResult();
        pp.SetPassThroughUnknownCmdsCB([](WStringView s) -> bool
          { return s == "version"; }); // TestSettings[i].m_bPassThroughUnknownCommands);

        {
          fileName.SetFormat("Preprocessor/{0}.txt", TestSettings[i].m_szFileName);
          fileNameExp.SetFormat("Preprocessor/{0} - Expected.txt", TestSettings[i].m_szFileName);
          fileNameOut.SetFormat(":output/Preprocessor/{0} - Result.txt", TestSettings[i].m_szFileName);

          W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "File does not exist: '%s'", fileName.GetData());

          WFileWriter fout;
          W_VERIFY(fout.Open(fileNameOut).Succeeded(), "Could not create output file '{0}'", fileNameOut);

          if (pp.Process(fileName, sOutput) == W_SUCCESS)
          {
            WString sError = "Processing succeeded\r\n";
            fout.WriteBytes(sError.GetData(), sError.GetElementCount()).IgnoreResult();
            fout.WriteBytes(sOutput.GetData(), sOutput.GetElementCount()).IgnoreResult();

            if (!log.m_sOutput.IsEmpty())
              fout.WriteBytes("\r\n", 2).IgnoreResult();
          }
          else
          {
            WString sError = "Processing failed\r\n";
            fout.WriteBytes(sError.GetData(), sError.GetElementCount()).IgnoreResult();
          }

          fout.WriteBytes(log.m_sOutput.GetData(), log.m_sOutput.GetElementCount()).IgnoreResult();

          W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileNameOut), "Output file is missing: '%s'", fileNameOut.GetData());
        }

        W_TEST_TEXT_FILES(fileNameOut.GetData(), fileNameExp.GetData(), "");
      }
    }
  }


  WFileSystem::RemoveDataDirectoryGroup("PreprocessorTest");
}

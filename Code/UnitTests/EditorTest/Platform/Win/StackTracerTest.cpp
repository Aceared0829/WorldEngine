#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

W_CREATE_SIMPLE_TEST_GROUP(Platform);

W_CREATE_SIMPLE_TEST(Platform, StackTracer)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Parse StackTrace FileName & LineNumber")
  {
    WStringView sFileName;
    WInt32 lineNumber;

    // Happy path
    {
      WStringView sLogMessage = "  C:\\dev\\W engine\\Game\\GamePlugin\\Components\\TestComponent.cpp(238):'TestComponent::Update'";
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      W_TEST_STRING(sFileName, "C:\\dev\\W engine\\Game\\GamePlugin\\Components\\TestComponent.cpp");
      W_TEST_INT(lineNumber, 238);
    }

    // Funky file name
    {
      WStringView sLogMessage = "  C:\\dev\\worldengine\\Game\\GamePlugin\\Components\\funkynam1 e..(238)..cpp(238):'TestComponent::Update'";
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      W_TEST_STRING(sFileName, "C:\\dev\\worldengine\\Game\\GamePlugin\\Components\\funkynam1 e..(238)..cpp");
      W_TEST_INT(lineNumber, 238);
    }

    // UTF-8 Filename
    {
      WStringView sLogMessage = "  C:\\dev\\worldengine\\你好ÖÖÜÜê\\GamePlugin\\Components\\你好ÖÖÜÜê.cpp(238):'TestComponent::Update'";
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      W_TEST_STRING(sFileName, "C:\\dev\\worldengine\\你好ÖÖÜÜê\\GamePlugin\\Components\\你好ÖÖÜÜê.cpp");
      W_TEST_INT(lineNumber, 238);
    }

    // Normal Log Message
    {
      WStringView sLogMessage = "  WInputManager::GetInputSlotState: Input Slot 'mouse_position_x' does not exist (yet):...";
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Empty Log Message
    {
      WStringView sLogMessage = "";
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Invalid
    {
      WStringView sLogMessage;
      W_TEST_BOOL(WStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parse Assert FileName & LineNumber")
  {
    WStringView sFileName;
    WInt32 lineNumber;

    // Happy path
    {
      WStringView sLogMessage = "*** Assertion ***: File: \"C:\\dev\\W engine\\Game\\GamePlugin\\Components\\TestComponent.cpp\", Line: \"136\", Function: \"TestComponent::Update\", Expression: \"false\", Message: \"Assert!\"";
      W_TEST_BOOL(WStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      W_TEST_STRING(sFileName, "C:\\dev\\W engine\\Game\\GamePlugin\\Components\\TestComponent.cpp");
      W_TEST_INT(lineNumber, 136);
    }

    // Normal Log Message
    {
      WStringView sLogMessage = "  WInputManager::GetInputSlotState: Input Slot 'mouse_position_x' does not exist (yet):...";
      W_TEST_BOOL(WStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Broken log message
    {
      WStringView sLogMessage = "*** Assertion ***: File: \"C:\\dev\\W engine\\Game\\GamePlugin\\Components\\TestComponent.cpp\", Line: \"12";
      W_TEST_BOOL(WStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }
  }
}


#endif
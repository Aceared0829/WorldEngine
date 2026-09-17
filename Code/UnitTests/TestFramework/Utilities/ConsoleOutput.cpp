
#include <TestFramework/Framework/TestFramework.h>

#include <ConsoleOutput_Platform.inl>

void OutputToConsole(WTestOutput::Enum type, const char* szMsg)
{
  static WInt32 iIndentation = 0;
  static bool bAnyError = false;

  WUInt8 uiColor = 0x07;

  switch (type)
  {
    case WTestOutput::StartOutput:
      break;
    case WTestOutput::BeginBlock:
      iIndentation += 2;
      break;
    case WTestOutput::EndBlock:
      iIndentation -= 2;
      break;
    case WTestOutput::Details:
      break;
    case WTestOutput::ImportantInfo:
      break;
    case WTestOutput::Success:
      uiColor = 0x0A;
      break;
    case WTestOutput::Message:
      uiColor = 0x0E;
      break;
    case WTestOutput::Warning:
      uiColor = 0x0C;
      break;
    case WTestOutput::Error:
      uiColor = 0x0C;
      bAnyError = true;
      break;
    case WTestOutput::Duration:
    case WTestOutput::ImageDiffFile:
    case WTestOutput::InvalidType:
    case WTestOutput::AllOutputTypes:
      return;

    case WTestOutput::FinalResult:
      if (bAnyError)
        uiColor = 0x0C;
      else
        uiColor = 0x0A;

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  OutputToConsole_Platform(uiColor, type, iIndentation, szMsg);

  if (type >= WTestOutput::Error)
  {
    fflush(stdout);
  }
}

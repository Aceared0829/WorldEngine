#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/ConversionUtils.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

bool WDefaultAssertHandler_Platform(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp);

#include <Assert_Platform.inl>

bool WDefaultAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  char szTemp[1024 * 4] = "";
  WStringUtils::snprintf(szTemp, W_ARRAY_SIZE(szTemp),
    "\n\n *** Assertion ***\n\n    Expression: \"%s\"\n    Function: \"%s\"\n    File: \"%s\"\n    Line: %u\n    Message: \"%s\"\n\n", szExpression,
    szFunction, szSourceFile, uiLine, szAssertMsg);
  szTemp[1024 * 4 - 1] = '\0';

  WLog::Print(szTemp);

  if (WSystemInformation::IsDebuggerAttached())
    return true;

  // If no debugger is attached we append the assert to a common file so that postmortem debugging is easier
  if (FILE* assertLogFP = fopen("WDefaultAssertHandlerOutput.txt", "a"))
  {
    time_t timeUTC = time(&timeUTC);
    tm* ptm = gmtime(&timeUTC);

    char szTimeStr[256] = {0};
    WStringUtils::snprintf(szTimeStr, 256, "UTC: %s", asctime(ptm));
    fputs(szTimeStr, assertLogFP);

    fputs(szTemp, assertLogFP);

    fclose(assertLogFP);
  }

  // if the environment variable "W_SILENT_ASSERTS" is set to a value like "1", "on", "true", "enable" or "yes"
  // the assert handler will never show a GUI that may block the application from continuing to run
  // this should be set on machines that run tests which should never get stuck but rather crash asap
  bool bSilentAsserts = false;

  if (WEnvironmentVariableUtils::IsVariableSet("W_SILENT_ASSERTS"))
  {
    bSilentAsserts = WEnvironmentVariableUtils::GetValueInt("W_SILENT_ASSERTS", bSilentAsserts ? 1 : 0) != 0;
  }

  if (bSilentAsserts)
    return true;

  return WDefaultAssertHandler_Platform(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg, szTemp);
}

static WAssertHandler g_AssertHandler = &WDefaultAssertHandler;

WAssertHandler WGetAssertHandler()
{
  return g_AssertHandler;
}

void WSetAssertHandler(WAssertHandler handler)
{
  g_AssertHandler = handler;
}

bool WFailedCheck(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}

bool WFailedCheck(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const class WFormatString& msg)
{
  WStringBuilder tmp;
  return WFailedCheck(szSourceFile, uiLine, szFunction, szExpression, msg.GetTextCStr(tmp));
}

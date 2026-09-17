#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

WApplication::WApplication(WStringView sAppName)
  : m_sAppName(sAppName)
{
}

WApplication::~WApplication() = default;

void WApplication::SetApplicationName(WStringView sAppName)
{
  m_sAppName = sAppName;
}

WCommandLineOptionBool opt_WaitForDebugger("app", "-WaitForDebugger", "If specified, the application will wait at startup until a debugger is attached.", false);

WResult WApplication::BeforeCoreSystemsStartup()
{
  if (WFileSystem::DetectSdkRootDirectory().Failed())
  {
    WLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  WRTTI::VerifyCorrectnessForAllTypes();
#endif

  if (opt_WaitForDebugger.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    while (!WSystemInformation::IsDebuggerAttached())
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));
    }

    W_DEBUG_BREAK;
  }

  return W_SUCCESS;
}


void WApplication::SetCommandLineArguments(WUInt32 uiArgumentCount, const char** pArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_pArguments = pArguments;

  WCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, pArguments, WCommandLineUtils::PreferOsArgs);
}


const char* WApplication::GetArgument(WUInt32 uiArgument) const
{
  W_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_pArguments[uiArgument];
}


WApplication* WApplication::s_pApplicationInstance = nullptr;

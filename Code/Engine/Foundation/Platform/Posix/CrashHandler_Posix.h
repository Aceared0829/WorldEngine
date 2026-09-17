#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/StackTracer.h>

#include <csignal>
#include <cxxabi.h>
#include <unistd.h>

static void PrintHelper(const char* szString)
{
  WLog::Printf("%s", szString);
}

static void WCrashHandlerFunc() noexcept
{
  if (WCrashHandler::GetCrashHandler() != nullptr)
  {
    WCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // restore the original signal handler for the abort signal and raise one so the kernel can do a core dump
  std::signal(SIGABRT, SIG_DFL);
  std::raise(SIGABRT);
}

static void WSignalHandler(int signum)
{
  WLog::Printf("***Unhandled Signal:***\n");
  switch (signum)
  {
    case SIGINT:
      WLog::Printf("Signal SIGINT: interrupt\n");
      break;
    case SIGILL:
      WLog::Printf("Signal SIGILL: illegal instruction - invalid function image\n");
      break;
    case SIGFPE:
      WLog::Printf("Signal SIGFPE: floating point exception\n");
      break;
    case SIGSEGV:
      WLog::Printf("Signal SIGSEGV: segment violation\n");
      break;
    case SIGTERM:
      WLog::Printf("Signal SIGTERM: Software termination signal from kill\n");
      break;
    case SIGABRT:
      WLog::Printf("Signal SIGABRT: abnormal termination triggered by abort call\n");
      break;
    default:
      WLog::Printf("Signal %i: unknown signal\n", signal);
      break;
  }

  if (WCrashHandler::GetCrashHandler() != nullptr)
  {
    WCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // forward the signal back to the OS so that it can write a core dump
  std::signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void WCrashHandler::SetCrashHandler(WCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    std::signal(SIGINT, WSignalHandler);
    std::signal(SIGILL, WSignalHandler);
    std::signal(SIGFPE, WSignalHandler);
    std::signal(SIGSEGV, WSignalHandler);
    std::signal(SIGTERM, WSignalHandler);
    std::signal(SIGABRT, WSignalHandler);
    std::set_terminate(WCrashHandlerFunc);
  }
  else
  {
    std::signal(SIGINT, nullptr);
    std::signal(SIGILL, nullptr);
    std::signal(SIGFPE, nullptr);
    std::signal(SIGSEGV, nullptr);
    std::signal(SIGTERM, nullptr);
    std::signal(SIGABRT, nullptr);
    std::set_terminate(nullptr);
  }
}

bool WCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
#if W_ENABLED(W_SUPPORTS_CRASH_DUMPS)
  WStatus res = WMiniDumpUtils::WriteOwnProcessMiniDump(m_sDumpFilePath, pOsSpecificData);
  if (res.Failed())
    WLog::Printf("WriteOwnProcessMiniDump failed: %s\n", res.GetMessageString().GetData());
  return res.Succeeded();
#else
  return false;
#endif
}

void WCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  WLog::Printf("***Unhandled Exception:***\n");

  // WLog::Printf exception type
  if (std::type_info* type = abi::__cxa_current_exception_type())
  {
    if (const char* szName = type->name())
    {
      int status = -1;
      // Try to print nice name
      if (char* szNiceName = abi::__cxa_demangle(szName, 0, 0, &status))
        WLog::Printf("Exception: %s\n", szNiceName);
      else
        WLog::Printf("Exception: %s\n", szName);
    }
  }

  {
    WLog::Printf("\n\n***Stack Trace:***\n");

    void* pBuffer[64];
    WArrayPtr<void*> tempTrace(pBuffer);
    const WUInt32 uiNumTraces = WStackTracer::GetStackTrace(tempTrace);

    WStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}

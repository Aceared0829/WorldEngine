#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/System/CrashHandler.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/StackTracer.h>

static void PrintHelper(const char* szString)
{
  WLog::Printf("%s", szString);
}

static LONG WINAPI WCrashHandlerFunc(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  static WMutex s_CrashMutex;
  W_LOCK(s_CrashMutex);

  static bool s_bAlreadyHandled = false;

  if (s_bAlreadyHandled == false)
  {
    if (WCrashHandler::GetCrashHandler() != nullptr)
    {
      s_bAlreadyHandled = true;
      WCrashHandler::GetCrashHandler()->HandleCrash(pExceptionInfo);
    }
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void WCrashHandler::SetCrashHandler(WCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    SetUnhandledExceptionFilter(WCrashHandlerFunc);
  }
  else
  {
    SetUnhandledExceptionFilter(nullptr);
  }
}

bool WCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  WStatus res = WMiniDumpUtils::WriteOwnProcessMiniDump(m_sDumpFilePath, (_EXCEPTION_POINTERS*)pOsSpecificData);
  if (res.Failed())
    WLog::Printf("WriteOwnProcessMiniDump failed: %s\n", res.GetMessageString().GetData());
  return res.Succeeded();
#  else
  W_IGNORE_UNUSED(pOsSpecificData);
  return false;
#  endif
}

void WCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  _EXCEPTION_POINTERS* pExceptionInfo = (_EXCEPTION_POINTERS*)pOsSpecificData;

  WLog::Printf("***Unhandled Exception:***\n");
  WLog::Printf("Exception: %08x", (WUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

  {
    WLog::Printf("\n\n***Stack Trace:***\n");
    void* pBuffer[64];
    WArrayPtr<void*> tempTrace(pBuffer);
    const WUInt32 uiNumTraces = WStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

    WStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}

#endif

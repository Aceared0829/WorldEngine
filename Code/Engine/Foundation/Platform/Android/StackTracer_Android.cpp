#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/System/StackTracer.h>

#  include <dlfcn.h>
#  include <unwind.h>

void WStackTracer::OnPluginEvent(const WPluginEvent& e) {}

struct Backtrace
{
  WUInt32 uiPos = 0;
  WArrayPtr<void*> trace;
};

static _Unwind_Reason_Code BacktraceCallback(struct _Unwind_Context* pContext, void* pData)
{
  Backtrace& backtrace = *(Backtrace*)pData;

  if (backtrace.uiPos < backtrace.trace.GetCount())
  {
    backtrace.trace[backtrace.uiPos] = reinterpret_cast<void*>(_Unwind_GetIP(pContext));
    backtrace.uiPos++;
    return _URC_NO_REASON;
  }
  else
  {
    return _URC_END_OF_STACK;
  }
}
// static
WUInt32 WStackTracer::GetStackTrace(WArrayPtr<void*>& trace, void* pContext)
{
  Backtrace backtrace;
  backtrace.trace = trace;
  _Unwind_Reason_Code res = _Unwind_Backtrace(BacktraceCallback, &backtrace);
  W_IGNORE_UNUSED(res);
  return backtrace.uiPos;
}

// static
void WStackTracer::ResolveStackTrace(const WArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];
  for (WUInt32 i = 0; i < trace.GetCount(); i++)
  {
    Dl_info info;
    if (dladdr(trace[i], &info) && info.dli_sname)
    {
      int iLen = WMath::Min(strlen(info.dli_sname), (size_t)W_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, info.dli_sname, iLen);
      szBuffer[iLen] = '\n';
      szBuffer[iLen + 1] = '\0';
      printFunc(szBuffer);
    }
    else
    {
      printFunc("Unresolved stack.\n");
    }
  }
}

#endif

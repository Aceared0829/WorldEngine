#include <Foundation/System/StackTracer.h>

void WStackTracer::OnPluginEvent(const WPluginEvent& e)
{
}

WUInt32 WStackTracer::GetStackTrace(WArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

void WStackTracer::ResolveStackTrace(const WArrayPtr<void*>& trace, PrintFunc printFunc)
{
}

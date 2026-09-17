#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/StackTracer.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, StackTracer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPlugin::Events().AddEventHandler(WStackTracer::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPlugin::Events().RemoveEventHandler(WStackTracer::OnPluginEvent);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WStackTracer::PrintStackTrace(const WArrayPtr<void*>& trace, WStackTracer::PrintFunc printFunc)
{
  char buffer[32];
  const WUInt32 uiNumTraceEntries = trace.GetCount();
  for (WUInt32 i = 0; i < uiNumTraceEntries; i++)
  {
    WStringUtils::snprintf(buffer, W_ARRAY_SIZE(buffer), "%s%p", i == 0 ? "" : "|", trace[i]);
    printFunc(buffer);
  }
}

W_STATICLINK_FILE(Foundation, Foundation_System_Implementation_StackTracer);

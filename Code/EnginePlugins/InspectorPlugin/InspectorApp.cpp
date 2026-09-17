#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Utilities/Stats.h>

static WAssertHandler g_PreviousAssertHandler = nullptr;

static bool TelemetryAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (WTelemetry::IsConnectedToClient())
  {
    WTelemetryMessage msg;
    msg.SetMessageID(' APP', 'ASRT');
    msg.GetWriter() << szSourceFile;
    msg.GetWriter() << uiLine;
    msg.GetWriter() << szFunction;
    msg.GetWriter() << szExpression;
    msg.GetWriter() << szAssertMsg;

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);

    // messages might not arrive, if the network does not get enough time to transmit them
    // since we are crashing the application in (half) 'a second', we need to make sure the network traffic has indeed been sent
    for (WUInt32 i = 0; i < 5; ++i)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
      WTelemetry::UpdateNetwork();
    }
  }

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void AddTelemetryAssertHandler()
{
  g_PreviousAssertHandler = WGetAssertHandler();
  WSetAssertHandler(TelemetryAssertHandler);
}

void RemoveTelemetryAssertHandler()
{
  WSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void SetAppStats()
{
  WStringBuilder sOut;
  const WSystemInformation info = WSystemInformation::Get();

  WStats::SetStat("Platform/Name", info.GetPlatformName());

  WStats::SetStat("Hardware/CPU Cores", info.GetCPUCoreCount());

  WStats::SetStat("Hardware/RAM[GB]", info.GetInstalledMainMemory() / 1024.0f / 1024.0f / 1024.0f);

  sOut = info.Is64BitOS() ? "64 Bit" : "32 Bit";
  WStats::SetStat("Platform/Architecture", sOut.GetData());

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  sOut = "Debug";
#elif W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  sOut = "Dev";
#else
  sOut = "Release";
#endif
  WStats::SetStat("Platform/Build", sOut.GetData());

#if W_ENABLED(W_USE_PROFILING)
  sOut = "Enabled";
#else
  sOut = "Disabled";
#endif
  WStats::SetStat("Features/Profiling", sOut.GetData());

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  WStats::SetStat("Features/Allocation Tracking", sOut.GetData());

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStatsAndStacktraces)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  WStats::SetStat("Features/Allocation Stack Tracing", sOut.GetData());

#if W_ENABLED(W_PLATFORM_LITTLE_ENDIAN)
  sOut = "Little";
#else
  sOut = "Big";
#endif
  WStats::SetStat("Platform/Endianess", sOut.GetData());
}



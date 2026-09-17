#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_OSX)

#  include <Foundation/System/MiniDumpUtils.h>

WStatus WMiniDumpUtils::WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WDumpType dumpTypeOverride)
{
  return WStatus("Not implemented on OSX");
}

WStatus WMiniDumpUtils::LaunchMiniDumpTool(WStringView sDumpFile, WDumpType dumpTypeOverride)
{
  return WStatus("Not implemented on OSX");
}

#endif

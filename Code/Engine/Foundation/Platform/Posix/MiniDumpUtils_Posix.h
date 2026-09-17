#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

WStatus WMiniDumpUtils::WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WDumpType dumpTypeOverride)
{
  return WStatus("Not implemented on Posix");
}

WStatus WMiniDumpUtils::LaunchMiniDumpTool(WStringView sDumpFile, WDumpType dumpTypeOverride)
{
  return WStatus("Not implemented on Posix");
}

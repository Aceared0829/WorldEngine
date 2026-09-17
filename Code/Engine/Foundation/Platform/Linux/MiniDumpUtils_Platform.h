#pragma once

namespace WMiniDumpUtils
{
  /// Linux-specific implementation for writing a core dump of the running process.
  ///
  /// This triggers gcore or uses the kernel's core dump mechanism.
  W_FOUNDATION_DLL WStatus WriteOwnProcessMiniDump(WStringView sDumpFile, void* pOsSpecificData, WDumpType dumpTypeOverride = WDumpType::Auto);

}; // namespace WMiniDumpUtils

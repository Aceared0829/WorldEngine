#pragma once

#include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  struct _EXCEPTION_POINTERS;
}

namespace WMiniDumpUtils
{
  /// Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  W_FOUNDATION_DLL WStatus WriteOwnProcessMiniDump(WStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, WDumpType dumpTypeOverride = WDumpType::Auto);

  /// Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  W_FOUNDATION_DLL WMinWindows::HANDLE GetProcessHandleWithNecessaryRights(WUInt32 uiProcessID);

  /// Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  W_FOUNDATION_DLL WStatus WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WMinWindows::HANDLE hProcess, WDumpType dumpTypeOverride = WDumpType::Auto);

  /// Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: A crash-dump with a full memory capture is made if either this application's command line option '-fullcrashdumps' is specified or if that setting is overridden through dumpTypeOverride = WDumpType::MiniDumpWithFullMemory.
  W_FOUNDATION_DLL WStatus WriteProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, WDumpType dumpTypeOverrideType = WDumpType::Auto);

}; // namespace WMiniDumpUtils

#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <Foundation/Utilities/CommandLineOptions.h>
#  include <Foundation/Utilities/CommandLineUtils.h>

#  include <Dbghelp.h>
#  include <Shlwapi.h>
#  include <tchar.h>
#  include <werapi.h>

WCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

using MINIDUMPWRITEDUMP = BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

WMinWindows::HANDLE WMiniDumpUtils::GetProcessHandleWithNecessaryRights(WUInt32 uiProcessID)
{
  // try to get more than we need
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uiProcessID);

  if (hProcess == NULL)
  {
    // try to get all that we need for a nice dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, uiProcessID);
  }

  if (hProcess == NULL)
  {
    // try to get rights for a limited dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uiProcessID);
  }

  return hProcess;
}

WStatus WMiniDumpUtils::WriteProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, WDumpType dumpTypeOverride)
{
  HMODULE hDLL = ::LoadLibraryA("dbghelp.dll");

  if (hDLL == nullptr)
  {
    return WStatus("dbghelp.dll could not be loaded.");
  }

  MINIDUMPWRITEDUMP MiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMP)::GetProcAddress(hDLL, "MiniDumpWriteDump");

  if (MiniDumpWriteDumpFunc == nullptr)
  {
    return WStatus("'MiniDumpWriteDump' function address could not be resolved.");
  }

  WUInt32 dumpType = MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
                      MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo;

  if ((opt_FullCrashDumps.GetOptionValue(WCommandLineOption::LogMode::Always) && dumpTypeOverride == WDumpType::Auto) || dumpTypeOverride == WDumpType::MiniDumpWithFullMemory)
  {
    dumpType |= MiniDumpWithFullMemory;
  }

  // make sure the target folder exists
  {
    WStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (WOSFile::CreateDirectoryStructure(folder).Failed())
      return WStatus("Failed to create output directory structure.");
  }

  HANDLE hFile = CreateFileW(WDosDevicePath(sDumpFile), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return WStatus(WFmt("Creating dump file '{}' failed (Error: '{}').", sDumpFile, WArgErrorCode(GetLastError())));
  }

  W_SCOPE_EXIT(CloseHandle(hFile););

  MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
  exceptionParam.ThreadId = GetCurrentThreadId(); // only valid for WriteOwnProcessMiniDump()
  exceptionParam.ExceptionPointers = pExceptionInfo;
  exceptionParam.ClientPointers = TRUE;

  if (MiniDumpWriteDumpFunc(
        hProcess, uiProcessID, hFile, (MINIDUMP_TYPE)dumpType, pExceptionInfo != nullptr ? &exceptionParam : nullptr, nullptr, nullptr) == FALSE)
  {
    return WStatus(WFmt("Writing dump file failed: '{}'.", WArgErrorCode(GetLastError())));
  }

  return W_SUCCESS;
}

WStatus WMiniDumpUtils::WriteOwnProcessMiniDump(WStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, WDumpType dumpTypeOverride)
{
  return WriteProcessMiniDump(sDumpFile, GetCurrentProcessId(), GetCurrentProcess(), pExceptionInfo, dumpTypeOverride);
}

WStatus WMiniDumpUtils::WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WMinWindows::HANDLE hProcess, WDumpType dumpTypeOverride)
{
  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr, dumpTypeOverride);
}

WStatus WMiniDumpUtils::WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WDumpType dumpTypeOverride)
{
  HANDLE hProcess = WMiniDumpUtils::GetProcessHandleWithNecessaryRights(uiProcessID);

  if (hProcess == nullptr)
  {
    return WStatus("Cannot access process for mini-dump writing (PID invalid or not enough rights).");
  }

  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr, dumpTypeOverride);
}

WStatus WMiniDumpUtils::LaunchMiniDumpTool(WStringView sDumpFile, WDumpType dumpTypeOverride)
{
  WStringBuilder sDumpToolPath = WOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("WMiniDumpTool.exe");
  sDumpToolPath.MakeCleanPath();

  if (!WOSFile::ExistsFile(sDumpToolPath))
    return WStatus(WFmt("WMiniDumpTool.exe not found in '{}'", sDumpToolPath));

  WProcessOptions procOpt;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", WProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(sDumpFile);

  if ((opt_FullCrashDumps.GetOptionValue(WCommandLineOption::LogMode::Always) && dumpTypeOverride == WDumpType::Auto) || dumpTypeOverride == WDumpType::MiniDumpWithFullMemory)
  {
    // forward the '-fullcrashdumps' command line argument
    procOpt.AddArgument("-fullcrashdumps");
  }

  WProcessGroup proc;
  if (proc.Launch(procOpt).Failed())
    return WStatus(WFmt("Failed to launch '{}'", sDumpToolPath));

  if (proc.WaitToFinish().Failed())
    return WStatus("Waiting for WMiniDumpTool to finish failed.");

  return W_SUCCESS;
}

#endif

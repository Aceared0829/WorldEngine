#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Timestamp.h>

//////////////////////////////////////////////////////////////////////////

WCrashHandler* WCrashHandler::s_pActiveHandler = nullptr;

WCrashHandler::WCrashHandler() = default;

WCrashHandler::~WCrashHandler()
{
  if (s_pActiveHandler == this)
  {
    SetCrashHandler(nullptr);
  }
}

WCrashHandler* WCrashHandler::GetCrashHandler()
{
  return s_pActiveHandler;
}

//////////////////////////////////////////////////////////////////////////

WCrashHandler_WriteMiniDump WCrashHandler_WriteMiniDump::g_Instance;

WCrashHandler_WriteMiniDump::WCrashHandler_WriteMiniDump() = default;

void WCrashHandler_WriteMiniDump::SetFullDumpFilePath(WStringView sFullAbsDumpFilePath)
{
  m_sDumpFilePath = sFullAbsDumpFilePath;
}

void WCrashHandler_WriteMiniDump::SetDumpFilePath(WStringView sAbsDirectoryPath, WStringView sAppName, WBitflags<PathFlags> flags)
{
  WStringBuilder sOutputPath = sAbsDirectoryPath;

  if (flags.IsSet(PathFlags::AppendSubFolder))
  {
    sOutputPath.AppendPath("CrashDumps");
  }

  sOutputPath.AppendPath(sAppName);

  if (flags.IsSet(PathFlags::AppendDate))
  {
    const WDateTime date = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());
    sOutputPath.AppendFormat("_{}", date);
  }

#if W_ENABLED(W_SUPPORTS_PROCESSES)
  if (flags.IsSet(PathFlags::AppendPID))
  {
    const WUInt32 pid = WProcess::GetCurrentProcessID();
    sOutputPath.AppendFormat("_{}", pid);
  }
#endif

  sOutputPath.Append(".dmp");

  SetFullDumpFilePath(sOutputPath);
}

void WCrashHandler_WriteMiniDump::SetDumpFilePath(WStringView sAppName, WBitflags<PathFlags> flags)
{
  SetDumpFilePath(WOSFile::GetApplicationDirectory(), sAppName, flags);
}

void WCrashHandler_WriteMiniDump::HandleCrash(void* pOsSpecificData)
{
  bool crashDumpWritten = false;
  if (!m_sDumpFilePath.IsEmpty())
  {
#if W_ENABLED(W_SUPPORTS_CRASH_DUMPS)
    if (WMiniDumpUtils::LaunchMiniDumpTool(m_sDumpFilePath).Failed())
    {
      WLog::Print("Could not launch MiniDumpTool, trying to write crash-dump from crashed process directly.\n");

      crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
    }
    else
    {
      crashDumpWritten = true;
    }
#else
    crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
#endif
  }
  else
  {
    WLog::Print("WCrashHandler_WriteMiniDump: No dump-file location specified.\n");
  }

  PrintStackTrace(pOsSpecificData);

  if (crashDumpWritten)
  {
    WLog::Printf("Application crashed. Crash-dump written to '%s'\n.", m_sDumpFilePath.GetData());
  }
}

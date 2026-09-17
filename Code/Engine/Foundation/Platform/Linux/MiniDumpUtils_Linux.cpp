#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/Process.h>
#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Utilities/CommandLineOptions.h>

#  include <sys/wait.h>
#  include <unistd.h>

WCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

WStatus WMiniDumpUtils::WriteExternalProcessMiniDump(WStringView sDumpFile, WUInt32 uiProcessID, WDumpType dumpTypeOverride)
{
  // Create the output directory if needed
  {
    WStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (WOSFile::CreateDirectoryStructure(folder).Failed())
      return WStatus("Failed to create output directory structure.");
  }

  // gcore outputs to <dumpfile>.<pid> format, so we need to handle the filename
  WStringBuilder sCoreName = sDumpFile;
  sCoreName.RemoveFileExtension();

  // Run gcore to generate the core dump
  // gcore -o <output_prefix> <pid>
  WProcessOptions procOpt;
  procOpt.m_bHideConsoleWindow = true;
  procOpt.m_sProcess = "gcore";
  procOpt.m_Arguments.PushBack("-o");
  procOpt.m_Arguments.PushBack(sCoreName);
  procOpt.AddArgument("{}", uiProcessID);

  WInt32 iExitCode = -1;
  if (WProcess::Execute(procOpt, &iExitCode).Failed())
  {
    return WStatus("gcore not found. Install gdb package to enable crash dump support.");
  }

  if (iExitCode != 0)
  {
    return WStatus(WFmt("gcore failed with exit code {}", iExitCode));
  }

  // gcore creates file as <prefix>.<pid>, rename to requested name
  WStringBuilder sGcoreOutput;
  sGcoreOutput.SetFormat("{}.{}", sCoreName, uiProcessID);

  if (WOSFile::ExistsFile(sGcoreOutput))
  {
    if (sGcoreOutput != sDumpFile)
    {
      WOSFile::MoveFileOrDirectory(sGcoreOutput, sDumpFile).IgnoreResult();
    }
  }

  return W_SUCCESS;
}

WStatus WMiniDumpUtils::WriteOwnProcessMiniDump(WStringView sDumpFile, void* pOsSpecificData, WDumpType dumpTypeOverride)
{
  // Writing a dump of our own process is tricky because we're potentially in a crashed state.
  // The preferred approach is LaunchMiniDumpTool. This function is a fallback.

  // Create the output directory if needed
  {
    WStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (WOSFile::CreateDirectoryStructure(folder).Failed())
      return WStatus("Failed to create output directory structure.");
  }

  pid_t myPid = getpid();

  // Fork a child to run gcore on us
  pid_t childPid = fork();
  if (childPid == 0)
  {
    // Child process - run gcore on parent
    WStringBuilder sCoreName = sDumpFile;
    sCoreName.RemoveFileExtension();

    WStringBuilder sPidArg;
    sPidArg.SetFormat("{}", myPid);

    execlp("gcore", "gcore", "-o", sCoreName.GetData(), sPidArg.GetData(), nullptr);
    _exit(1); // execlp failed
  }
  else if (childPid > 0)
  {
    // Parent - wait for child
    int status;
    waitpid(childPid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      // gcore succeeded, rename file from <prefix>.<pid> to requested name
      WStringBuilder sCoreName = sDumpFile;
      sCoreName.RemoveFileExtension();

      WStringBuilder sGcoreOutput;
      sGcoreOutput.SetFormat("{}.{}", sCoreName, myPid);

      if (WOSFile::ExistsFile(sGcoreOutput))
      {
        if (sGcoreOutput != sDumpFile)
        {
          WOSFile::MoveFileOrDirectory(sGcoreOutput, sDumpFile).IgnoreResult();
        }
        return W_SUCCESS;
      }
    }
  }

  return WStatus("Could not write mini dump - gcore not available or failed.");
}

WStatus WMiniDumpUtils::LaunchMiniDumpTool(WStringView sDumpFile, WDumpType dumpTypeOverride)
{
  WStringBuilder sDumpToolPath = WOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("WMiniDumpTool");
  sDumpToolPath.MakeCleanPath();

  if (!WOSFile::ExistsFile(sDumpToolPath))
    return WStatus(WFmt("WMiniDumpTool not found in '{}'", sDumpToolPath));

  WProcessOptions procOpt;
  procOpt.m_bHideConsoleWindow = true;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", WProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(sDumpFile);

  if ((opt_FullCrashDumps.GetOptionValue(WCommandLineOption::LogMode::Always) && dumpTypeOverride == WDumpType::Auto) || dumpTypeOverride == WDumpType::MiniDumpWithFullMemory)
  {
    // Forward the '-fullcrashdumps' command line argument
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

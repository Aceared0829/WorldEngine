#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

WCommandLineOptionInt opt_PID("_MiniDumpTool", "-PID", "Process ID of the application for which to create a crash dump.", 0);

WCommandLineOptionPath opt_DumpFile("_MiniDumpTool", "-f", "Path to the crash dump file to write.", "");

class WMiniDumpTool : public WApplication
{
  WUInt32 m_uiProcessID = 0;
  WStringBuilder m_sDumpFile;

public:
  using SUPER = WApplication;

  WMiniDumpTool()
    : WApplication("MiniDumpTool")
  {
  }

  WResult ParseArguments()
  {
    WCommandLineUtils* cmd = WCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");

    m_sDumpFile = opt_DumpFile.GetOptionValue(WCommandLineOption::LogMode::Always);
    m_sDumpFile.MakeCleanPath();

    if (m_uiProcessID == 0)
    {
      WLog::Error("Missing '-PID' argument");
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    WFileSystem::AddDataDirectory("", "App", ":", WDataDirUsage::AllowWrites).IgnoreResult();

    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void Run() override
  {
    {
      WStringBuilder cmdHelp;
      if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_MiniDumpTool"))
      {
        WLog::Print(cmdHelp);
        QuitApplication();
        return;
      }
    }

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      QuitApplication();
      return;
    }

    WMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID).IgnoreResult();
    QuitApplication();
  }
};

W_APPLICATION_ENTRY_POINT(WMiniDumpTool);

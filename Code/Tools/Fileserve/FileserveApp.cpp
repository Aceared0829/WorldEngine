#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>

void WFileserverApp::AfterCoreSystemsStartup()
{
  WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  WFileSystem::AddDataDirectory("", "App", ":", WDataDirUsage::AllowWrites).IgnoreResult();

  W_DEFAULT_NEW(WFileserver);

  WFileserver::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WFileserverApp::FileserverEventHandler, this));

#ifndef W_USE_QT
  WFileserver::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WFileserverApp::FileserverEventHandlerConsole, this));
  WFileserver::GetSingleton()->StartServer();
#endif

  // Load all available shader compiler plugins
  {
    WFileSystemIterator it;
    WStringBuilder sShaderCompilerSearch(WOSFile::GetApplicationDirectory(), "/WShaderCompiler*");
    sShaderCompilerSearch.MakeCleanPath();

    for (it.StartSearch(sShaderCompilerSearch, WFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
    {
      WStringBuilder sName = it.GetStats().m_sName;

      if (sName.HasExtension("DLL"))
      {
        sName.RemoveFileExtension();
        WPlugin::LoadPlugin(sName, WPluginLoadFlags::PluginIsOptional).IgnoreResult();
      }
    }
  }

  WFileserver::GetSingleton()->SetCustomMessageHandler('SHDR', WMakeDelegate(&WFileserverApp::ShaderMessageHandler, this));

  // TODO: CommandLine Option
  m_CloseAppTimeout = WTime::MakeFromSeconds(WCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_close_timeout", 0));
  m_TimeTillClosing = WTime::MakeFromSeconds(WCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_wait_timeout", 0));

  if (m_TimeTillClosing.GetSeconds() > 0)
  {
    m_TimeTillClosing += WTime::Now();
  }
}

void WFileserverApp::BeforeCoreSystemsShutdown()
{
  WFileserver::GetSingleton()->StopServer();

#ifndef W_USE_QT
  WFileserver::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WFileserverApp::FileserverEventHandlerConsole, this));
#endif

  WFileserver::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WFileserverApp::FileserverEventHandler, this));

  WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

void WFileserverApp::Run()
{
  // if there are no more connections, and we have a timeout to close when no connections are left, we return Quit
  if (m_uiConnections == 0 && m_TimeTillClosing > WTime::MakeFromSeconds(0) && WTime::Now() > m_TimeTillClosing)
  {
    QuitApplication();
    return;
  }

  if (WFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }
}

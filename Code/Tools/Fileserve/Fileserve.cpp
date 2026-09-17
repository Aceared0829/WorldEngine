#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#ifdef W_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <QApplication>
#  include <QFileDialog>
#  include <QSettings>
#endif

#ifdef W_USE_QT
int main(int iArgc, char** pArgv)
{
#else
int main(int iArgc, const char** pArgv)
{
#endif
  WFileserverApp* pApp = new WFileserverApp();

#ifdef W_USE_QT
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  WCommandLineUtils::GetGlobalInstance()->SetCommandLine();
#  else
  WCommandLineUtils::GetGlobalInstance()->SetCommandLine(iArgc, pArgv);
#  endif
  int dummyArgc = 0;
  char** dummyArgv = nullptr;
  QApplication* pQtApplication = new QApplication(dummyArgc, const_cast<char**>(dummyArgv));
  pQtApplication->setApplicationName("WFileserve");
  pQtApplication->setOrganizationDomain("www.ezEngine.net");
  pQtApplication->setOrganizationName("WorldEngine Project");
  pQtApplication->setApplicationVersion("1.0.0");

  if (WRun_Startup(pApp).Succeeded())
  {
    CreateFileserveMainWindow(pApp);
    pQtApplication->exec();
    WRun_Shutdown(pApp);
  }
#else
  pApp->SetCommandLineArguments((WUInt32)iArgc, pArgv);
  WRun(pApp);
#endif

  const int iReturnCode = pApp->GetReturnCode();
  if (iReturnCode != 0)
  {

    std::string text = pApp->TranslateReturnCode();
    if (!text.empty())
      printf("Return Code: '%s'\n", text.c_str());
  }

#ifdef W_USE_QT
  delete pQtApplication;
#endif

  delete pApp;


  return iReturnCode;
}

WResult WFileserverApp::BeforeCoreSystemsStartup()
{
  // before anything else: with '-help' the application only prints its options and exits, it must not
  // ask for a project folder and must not start serving
  {
    WStringBuilder cmdHelp;
    if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      WLog::Print(cmdHelp);
      SetReturnCode(-1);
      return W_FAILURE;
    }
  }

  WStartup::AddApplicationTag("tool");
  WStartup::AddApplicationTag("fileserve");

#ifdef W_USE_QT
  if (!WCommandLineUtils::GetGlobalInstance()->HasOption("-specialdirs"))
  {
    QString sLastFolder;

    {
      QSettings Settings;
      Settings.beginGroup(QLatin1String("Fileserve"));
      sLastFolder = Settings.value("LastProject", "").toString();
      Settings.endGroup();
    }

    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Project Folder", sLastFolder);
    if (!folder.isEmpty())
    {
      QSettings Settings;
      Settings.beginGroup(QLatin1String("Fileserve"));
      Settings.setValue("LastProject", folder);
      Settings.endGroup();

      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-specialdirs");
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("project");
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(folder.toUtf8().data());
    }
  }
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void WFileserverApp::FileserverEventHandler(const WFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case WFileserverEvent::Type::ClientConnected:
    case WFileserverEvent::Type::ClientReconnected:
      ++m_uiConnections;
      m_TimeTillClosing = WTime::MakeZero();
      break;
    case WFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = WTime::Now() + m_CloseAppTimeout;
      }

      break;
    default:
      break;
  }
}

void WFileserverApp::ShaderMessageHandler(WFileserveClientContext& ref_ctxt, WRemoteMessage& ref_msg, WRemoteInterface& ref_clientChannel, WDelegate<void(const char*)> logActivity)
{
  if (ref_msg.GetMessageID() == 'CMPL')
  {
    for (auto& dd : ref_ctxt.m_MountedDataDirs)
    {
      WFileSystem::AddDataDirectory(dd.m_sPathOnServer, "FileServe", dd.m_sRootName, WDataDirUsage::AllowWrites).IgnoreResult();
    }

    auto& r = ref_msg.GetReader();

    WStringBuilder tmp;
    WStringBuilder file, platform;
    WUInt32 numPermVars;
    WTempHybridArray<WPermutationVar, 16> permVars;

    r >> file;
    r >> platform;
    r >> numPermVars;
    permVars.SetCount(numPermVars);

    tmp.SetFormat("Compiling Shader '{}' - '{}'", file, platform);

    for (auto& pv : permVars)
    {
      r >> pv.m_sName;
      r >> pv.m_sValue;

      tmp.AppendWithSeparator(" | ", pv.m_sName, "=", pv.m_sValue);
    }

    logActivity(tmp);

    // enable runtime shader compilation and set the shader cache directories (this only works, if the user doesn't change the default values)
    // the 'active platform' value should never be used during shader compilation, because there it is passed in
    WShaderManager::Configure("FILESERVE_UNUSED", true);

    WLogSystemToBuffer log;
    WLogSystemScope ls(&log);

    WShaderCompiler sc;
    WResult res = sc.CompileShaderPermutationForPlatforms(file, permVars, WLog::GetThreadLocalLogSystem(), platform);

    WFileSystem::RemoveDataDirectoryGroup("FileServe");

    if (res.Succeeded())
    {
      // invalidate read cache to not short-circuit the next file read operation
      WRemoteMessage msg2('FSRV', 'INVC');
      ref_clientChannel.Send(WRemoteTransmitMode::Reliable, msg2);
    }
    else
    {
      logActivity("[ERROR] Shader Compilation failed:");

      WTempHybridArray<WStringView, 32> lines;
      log.m_sBuffer.Split(false, lines, "\n", "\r");

      for (auto line : lines)
      {
        tmp.Set(">   ", line);
        logActivity(tmp);
      }
    }

    {
      WRemoteMessage msg2('SHDR', 'CRES');
      msg2.GetWriter() << (res == W_SUCCESS);
      msg2.GetWriter() << log.m_sBuffer;

      ref_clientChannel.Send(WRemoteTransmitMode::Reliable, msg2);
    }
  }
}

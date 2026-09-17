#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

WString WQtEditorApp::FindToolApplication(const char* szToolName)
{
  WStringBuilder toolExe = szToolName;

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szToolName = toolExe;

  WEditorPreferencesUser* pPref = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  WTempHybridArray<WString, 3> sFolders;

  if (pPref->m_bUsePrecompiledTools)
  {
    if (!pPref->m_sCustomPrecompiledToolsFolder.IsEmpty() && WOSFile::ExistsDirectory(pPref->m_sCustomPrecompiledToolsFolder))
    {
      WStringBuilder customToolsFolder = pPref->m_sCustomPrecompiledToolsFolder;
      customToolsFolder.MakeCleanPath();
      sFolders.PushBack(customToolsFolder);
    }

    sFolders.PushBack(WApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));
    sFolders.PushBack(WApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
  }
  else
  {
    sFolders.PushBack(WApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
    sFolders.PushBack(WApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));
  }

  WStringBuilder sTool;
  for (auto& folder : sFolders)
  {
    sTool = folder;
    sTool.AppendPath(szToolName);

    if (WOSFile::ExistsFile(sTool))
      return sTool;
  }

  // just try the one in the same folder as the editor
  return szToolName;
}

WStatus WQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, WUInt32 uiSecondsTillTimeout, WLogInterface* pLogOutput /*= nullptr*/, WLogMsgType::Enum logLevel /*= WLogMsgType::InfoMsg*/, const char* szCWD /*= nullptr*/)
{
  // this block is supposed to be in the global log, not the given log interface
  W_LOG_BLOCK("Executing Tool", szTool);

  WStringBuilder toolExe = szTool;

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szTool = toolExe;

  WStringBuilder cmd;
  for (WInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  WLog::Debug("{}{}", szTool, cmd);


  QProcess proc;

  if (szCWD != nullptr)
  {
    proc.setWorkingDirectory(szCWD);
  }

  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]()
    { logoutput.append(proc.readAllStandardOutput()); });
  WString toolPath = WQtEditorApp::GetSingleton()->FindToolApplication(szTool);
  proc.start(QString::fromUtf8(toolPath, toolPath.GetElementCount()), arguments);

  if (!proc.waitForStarted(uiSecondsTillTimeout * 1000))
    return WStatus(WFmt("{0} could not be started", szTool));

  if (!proc.waitForFinished(uiSecondsTillTimeout * 1000))
    return WStatus(WFmt("{0} timed out", szTool));

  if (pLogOutput)
  {
    WStringBuilder tmp;

    struct LogBlockData
    {
      LogBlockData(WLogInterface* pInterface, const char* szName)
        : m_Name(szName)
        , m_Block(pInterface, m_Name)
      {
      }

      WString m_Name;
      WLogBlock m_Block;
    };

    WTempHybridArray<WUniquePtr<LogBlockData>, 8> blocks;

    QTextStream logoutputStream(&logoutput);
    while (!logoutputStream.atEnd())
    {
      tmp = logoutputStream.readLine().toUtf8().data();
      tmp.Trim(" \n");

      const char* szMsg = nullptr;
      WLogMsgType::Enum msgType = WLogMsgType::None;

      if (tmp.StartsWith("Error: "))
      {
        szMsg = &tmp.GetData()[7];
        msgType = WLogMsgType::ErrorMsg;
      }
      else if (tmp.StartsWith("Warning: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = WLogMsgType::WarningMsg;
      }
      else if (tmp.StartsWith("Seriously: "))
      {
        szMsg = &tmp.GetData()[11];
        msgType = WLogMsgType::SeriousWarningMsg;
      }
      else if (tmp.StartsWith("Success: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = WLogMsgType::SuccessMsg;
      }
      else if (tmp.StartsWith("+++++ "))
      {
        tmp.Trim("+ ");
        if (tmp.EndsWith("()"))
          tmp.Trim("() ");

        szMsg = tmp.GetData();
        blocks.PushBack(W_DEFAULT_NEW(LogBlockData, pLogOutput, szMsg));
        continue;
      }
      else if (tmp.StartsWith("----- "))
      {
        if (!blocks.IsEmpty())
          blocks.PopBack();

        continue;
      }
      else
      {
        szMsg = &tmp.GetData()[0];
        msgType = WLogMsgType::InfoMsg;

        // TODO: output all logged data in one big message, if the tool failed
      }

      if (msgType > logLevel || szMsg == nullptr)
        continue;

      WLog::BroadcastLoggingEvent(pLogOutput, msgType, szMsg);
    }

    blocks.Clear();
  }

  if (proc.exitStatus() == QProcess::ExitStatus::CrashExit)
  {
    return WStatus(WFmt("{0} crashed during execution", szTool));
  }
  else if (proc.exitCode() != 0)
  {
    return WStatus(WFmt("{0} returned error code {1}", szTool, proc.exitCode()));
  }

  return WStatus(W_SUCCESS);
}

WString WQtEditorApp::BuildFileserveCommandLine() const
{
  const WStringBuilder sToolPath = WQtEditorApp::GetSingleton()->FindToolApplication("WFileserve");
  const WStringBuilder sProjectDir = WToolsProject::GetSingleton()->GetProjectDirectory();
  WStringBuilder params;

  WStringBuilder cmd;
  cmd.Set(sToolPath, " -specialdirs project \"", sProjectDir, "\"");

  return cmd;
}

void WQtEditorApp::RunFileserve()
{
  const WStringBuilder sToolPath = WQtEditorApp::GetSingleton()->FindToolApplication("WFileserve");
  const WStringBuilder sProjectDir = WToolsProject::GetSingleton()->GetProjectDirectory();

  QStringList args;
  args << "-specialdirs"
       << "project" << sProjectDir.GetData() << "-fs_start";

  QProcess::startDetached(sToolPath.GetData(), args);
}

void WQtEditorApp::RunInspector(WUInt16 uiPort)
{
  const WStringBuilder sToolPath = WQtEditorApp::GetSingleton()->FindToolApplication("WInspector");
  QStringList args;

  if (uiPort != 0)
  {
    args << "-port" << QString::number(uiPort);
  }

  QProcess::startDetached(sToolPath.GetData(), args);
}

void WQtEditorApp::RunTracy()
{
#if BUILDSYSTEM_ENABLE_TRACY_SUPPORT == 0
  WQtUiServices::MessageBoxInformation("<html>This build of W was compiled without support for Tracy profiling.<br><br>See <a href='https://ezengine.net/pages/docs/debugging/tracy.html'>the documentation</a> for how to enable it.</html>");
#else
  const WStringBuilder sToolPath = WQtEditorApp::GetSingleton()->FindToolApplication("tracy-profiler");
  QStringList args;

  QProcess::startDetached(sToolPath.GetData(), args);
#endif
}

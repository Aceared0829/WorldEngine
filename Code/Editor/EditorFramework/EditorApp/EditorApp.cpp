#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QFileDialog>
#include <ToolsFoundation/Application/ApplicationServices.h>

WCommandLineOptionPath opt_RemoteProjectDir("_Editor", "-remoteProjectDir",
  "Directory into which a 'remote' project is downloaded, instead of asking the user for one.\n"
  "\n"
  "The project ends up in a subfolder named after the project. Opening a remote project needs a download\n"
  "location, which is normally picked in a folder dialog - without this option, opening one from a script\n"
  "fails. A project that was downloaded before is opened from wherever it was put, this option has no\n"
  "effect then.\n"
  "\n"
  "Example:\n"
  "  -project \"Data/Samples/Bistro\" -remoteProjectDir \"C:/Projects\"\n",
  "");

W_IMPLEMENT_SINGLETON(WQtEditorApp);

WEvent<const WEditorAppEvent&> WQtEditorApp::m_Events;

WQtEditorApp::WQtEditorApp()
  : m_SingletonRegistrar(this)
  , m_RecentProjects(20)
  , m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;
  m_pVersionChecker = W_DEFAULT_NEW(WQtVersionChecker);

  m_pTimer = new QTimer(nullptr);
  m_pAutoSaveTimer = new QTimer(nullptr);
}

WQtEditorApp::~WQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  delete m_pAutoSaveTimer;
  m_pAutoSaveTimer = nullptr;

  CloseSplashScreen();
}

WInt32 WQtEditorApp::RunEditor()
{
  WInt32 ret = m_pQtApplication->exec();
  return ret;
}

void WQtEditorApp::SlotTimedUpdate()
{
  if (WToolsProject::IsProjectOpen())
  {
    if (WEditorEngineProcessConnection::GetSingleton())
      WEditorEngineProcessConnection::GetSingleton()->Update();

    WAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  WTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged(false);

  m_pTimer->start(1);
}

void WQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void WQtEditorApp::SlotAutoSave()
{
  const auto* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  if (!pPreferences || pPreferences->m_uiAutoSaveMinutes == 0)
    return;

  const WTime tAutoSaveThreshold = WTime::MakeFromMinutes(pPreferences->m_uiAutoSaveMinutes);
  const WTime tNow = WTime::Now();

  // Find the oldest modified document that exceeds the auto-save threshold.
  WDocument* pOldestDoc = nullptr;
  WTime tOldestTime = tNow;

  for (auto pMan : WDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->WDocumentManager::GetAllOpenDocuments())
    {
      // Skip documents with an active transaction or undo/redo in progress (e.g. user is dragging something).
      // The auto-save timer will retry on the next tick.
      const WCommandHistory* pHistory = pDoc->GetCommandHistory();
      if (pHistory && (pHistory->IsInTransaction() || pHistory->IsInUndoRedo()))
        continue;

      // Documents without a window are not being edited by the user - they are opened programmatically, e.g. by the asset curator during a transform.
      // Auto-saving those would write changes to disk that whoever opened the document has not asked to save yet.
      if (!pDoc->HasWindowBeenRequested())
        continue;

      const WTime tModified = pDoc->GetModifiedTime();
      if (tModified.IsPositive() && (tNow - tModified) >= tAutoSaveThreshold && tModified < tOldestTime)
      {
        pOldestDoc = pDoc;
        tOldestTime = tModified;
      }
    }
  }

  if (pOldestDoc == nullptr)
    return;

  WQtDocumentWindow* pWnd = WQtDocumentWindow::FindWindowByDocument(pOldestDoc);
  if (pWnd && pWnd->GetDocument() == pOldestDoc)
  {
    pWnd->SaveDocument().IgnoreResult();
  }
  else
  {
    pOldestDoc->SaveDocument().IgnoreResult();
  }
}

void WQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  // Close the splash screen so it doesn't become the parent window of our message boxes.
  CloseSplashScreen();

  if (bForced || bNewVersionReleased)
  {
    if (m_pVersionChecker->IsLatestNewer())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation(
        WFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/ezEngine/ezEngine/releases\">Releases</A> for details.</html>",
          m_pVersionChecker->GetKnownLatestVersion(), m_pVersionChecker->GetOwnVersion()));
    }
    else
    {
      WStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_pVersionChecker->GetOwnVersion());

      WQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_pVersionChecker->IsLatestNewer())
  {
    WQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      WFmt("New version '{}' available, please update.", m_pVersionChecker->GetKnownLatestVersion()));
  }
}

void WQtEditorApp::EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case WEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (auto pTypeMsg = WDynamicCast<const WUpdateReflectionTypeMsgToEditor*>(e.m_pMsg))
      {
        WPhantomRttiManager::RegisterType(pTypeMsg->m_desc);
      }
      if (auto pDynEnumMsg = WDynamicCast<const WDynamicStringEnumMsgToEditor*>(e.m_pMsg))
      {
        auto& dynEnum = WDynamicStringEnum::CreateDynamicEnum(pDynEnumMsg->m_sEnumName);
        for (auto& sEnumValue : pDynEnumMsg->m_EnumValues)
        {
          dynEnum.AddValidValue(sEnumValue);
        }
        dynEnum.SortValues();
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<WProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

    case WEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      StoreEnginePluginModificationTimes();
      break;

    default:
      return;
  }
}

void WQtEditorApp::UiServicesEvents(const WQtUiServices::Event& e)
{
  if (e.m_Type == WQtUiServices::Event::Type::CheckForUpdates)
  {
    m_pVersionChecker->Check(true);
  }

  if (e.m_Type == WQtUiServices::Event::Type::GotoLinkTarget)
  {
    WStringView sTarget = e.m_sText;

    if (sTarget.TrimWordStart("asset:"))
    {
      WStringView sObjRef;

      if (const char* szRef = sTarget.FindSubString("#"))
      {
        sObjRef = WStringView(szRef + 1, sTarget.GetEndPointer());
        sTarget = WStringView(sTarget.GetStartPointer(), szRef);
      }

      if (WConversionUtils::IsStringUuid(sTarget))
      {
        const WUuid guid = WConversionUtils::ConvertStringToUuid(sTarget);

        WStringBuilder path;

        if (auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(guid))
        {
          path = pAsset->m_pAssetInfo->m_Path;
        }

        if (!path.IsEmpty())
        {
          if (WDocument* pDoc = OpenDocument(path, WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList))
          {
            WStringView sFilter = sObjRef;
            if (sFilter.TrimWordStart("filter:"))
            {
              // Strip surrounding quotes, if present.
              sFilter.TrimWordStart("\"");
              sFilter.TrimWordEnd("\"");

              if (WGameObjectDocument* pGameObjDoc = WDynamicCast<WGameObjectDocument*>(pDoc))
              {
                WGameObjectEvent filterEvent;
                filterEvent.m_Type = WGameObjectEvent::Type::TriggerSetScenegraphFilter;
                filterEvent.m_sPayload = sFilter;
                pGameObjDoc->m_GameObjectEvents.Broadcast(filterEvent);
              }
            }
            else if (WConversionUtils::IsStringUuid(sObjRef))
            {
              const WUuid objGuid = WConversionUtils::ConvertStringToUuid(sObjRef);

              const WDocumentObjectManager* pMan = pDoc->GetObjectManager();
              const WDocumentObject* pSelObj = pMan->GetObject(objGuid);

              // Walk up to the nearest selectable ancestor, e.g. a component's owning game object.
              while (pSelObj != nullptr && pMan->CanSelect(pSelObj).Failed())
              {
                pSelObj = pSelObj->GetParent();
              }

              if (pSelObj != nullptr)
              {
                pDoc->GetSelectionManager()->SetSelection(pSelObj);
              }
            }
          }
        }
      }
    }
  }
}

void WQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : WDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->WDocumentManager::GetAllOpenDocuments())
    {
      WQtDocumentWindow* pWnd = WQtDocumentWindow::FindWindowByDocument(pDoc);
      // Layers for example will share a window with the scene document and the window will always save the scene.
      if (pWnd && pWnd->GetDocument() == pDoc)
      {
        if (pWnd->SaveDocument().Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument().LogFailure();
      }
    }
  }
}

bool WQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void WQtEditorApp::OnDemandDynamicStringEnumLoad(WStringView sEnumName, WDynamicStringEnum& e)
{
  WStringBuilder sFile;
  sFile.SetFormat(":project/Editor/{}.txt", sEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(sEnumName);
}

bool ContainsPlugin(const WDynamicArray<WApplicationPluginConfig::PluginConfig>& all, const char* szPlugin)
{
  for (const WApplicationPluginConfig::PluginConfig& one : all)
  {
    if (one.m_sAppDirRelativePath == szPlugin)
      return true;
  }

  return false;
}

WResult WQtEditorApp::AddBundlesInOrder(WDynamicArray<WApplicationPluginConfig::PluginConfig>& order, const WPluginBundleSet& bundles, const WString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const
{
  const WPluginBundle& bundle = bundles.m_Plugins.Find(start).Value();

  for (const WString& req : bundle.m_RequiredBundles)
  {
    auto it = bundles.m_Plugins.Find(req);

    if (!it.IsValid())
    {
      WLog::Error("Plugin bundle '{}' has a dependency on bundle '{}' which does not exist.", start, req);
      return W_FAILURE;
    }

    W_SUCCEED_OR_RETURN(AddBundlesInOrder(order, bundles, req, bEditor, bEditorEngine, bRuntime));
  }

  if (bRuntime)
  {
    for (const WString& dll : bundle.m_RuntimePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        WApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditorEngine)
  {
    for (const WString& dll : bundle.m_EditorEnginePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        WApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditor)
  {
    for (const WString& dll : bundle.m_EditorPlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        WApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  return W_SUCCESS;
}

static void NormalizeDataDirPath(const WString& sPath, WStringBuilder& out_sClean)
{
  out_sClean = sPath;
  out_sClean.MakeCleanPath();

  while (out_sClean.EndsWith("/"))
    out_sClean.Shrink(0, 1);
}

static void CollectBundleDataDirsRecursive(const WPluginBundleSet& bundles, const WString& sBundleName, WSet<WString>& inout_visited, WSet<WString>& out_dirs)
{
  if (inout_visited.Contains(sBundleName))
    return;

  inout_visited.Insert(sBundleName);

  auto it = bundles.m_Plugins.Find(sBundleName);
  if (!it.IsValid())
    return;

  const WPluginBundle& bundle = it.Value();

  WStringBuilder sClean;
  for (const WString& sDir : bundle.m_DataDirectories)
  {
    NormalizeDataDirPath(sDir, sClean);
    out_dirs.Insert(sClean);
  }

  for (const WString& sReq : bundle.m_RequiredBundles)
  {
    CollectBundleDataDirsRecursive(bundles, sReq, inout_visited, out_dirs);
  }
}

void WQtEditorApp::GetActiveBundleDataDirectories(WSet<WString>& out_dirs) const
{
  WSet<WString> visited;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || it.Value().m_bSelected)
    {
      CollectBundleDataDirsRecursive(m_PluginBundles, it.Key(), visited, out_dirs);
    }
  }
}

void WQtEditorApp::GetAllKnownBundleDataDirectories(WSet<WString>& out_dirs) const
{
  WStringBuilder sClean;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    for (const WString& sDir : it.Value().m_DataDirectories)
    {
      NormalizeDataDirPath(sDir, sClean);
      out_dirs.Insert(sClean);
    }
  }
}

static WStatus ExtractArchive(const WString& sArchivePath)
{
  WStringBuilder sArchiveDir = sArchivePath;
  sArchiveDir.PathParentDirectory();
  sArchiveDir.TrimWordEnd("/");

  QStringList args;
  args << "x"
       << "-y" << WMakeQString(sArchivePath);

  return WQtEditorApp::GetSingleton()->ExecuteTool("7z", args, 30 * 60, nullptr, WLogMsgType::WarningMsg, sArchiveDir);
}

static WStatus ExtractArchivesInDirectory(const WStringBuilder& sDirectory)
{
  WDynamicArray<WString> archiveFiles;
  WFileSystemIterator it;
  for (it.StartSearch(sDirectory, WFileSystemIteratorFlags::ReportFilesRecursive); it.IsValid(); it.Next())
  {
    if (it.GetStats().m_sName.HasExtension("7z"))
    {
      WStringBuilder sFullPath;
      it.GetStats().GetFullPath(sFullPath);
      archiveFiles.PushBack(sFullPath);
    }
  }

  if (archiveFiles.IsEmpty())
    return W_SUCCESS;

  WProgressRange unpackProgress("Unpacking Archives", archiveFiles.GetCount(), true);

  for (const WString& sArchive : archiveFiles)
  {
    unpackProgress.BeginNextStep(WPathUtils::GetFileNameAndExtension(sArchive));

    if (unpackProgress.WasCanceled())
    {
      return WStatus("User canceled");
    }

    W_SUCCEED_OR_RETURN(ExtractArchive(sArchive));

    WLog::Success("Extracted archive '{}'", sArchive);
  }

  return W_SUCCESS;
}

WStatus WQtEditorApp::MakeRemoteProjectLocal(WStringBuilder& inout_sFilePath)
{
  // already a local project?
  if (inout_sFilePath.EndsWith_NoCase("WProject"))
    return WStatus(W_SUCCESS);

  {
    WStringBuilder tmp = inout_sFilePath;
    tmp.AppendPath("WProject");

    if (WOSFile::ExistsFile(tmp))
    {
      inout_sFilePath = tmp;
      return WStatus(W_SUCCESS);
    }
  }

  W_LOG_BLOCK("Open Remote Project", inout_sFilePath.GetData());

  WStringBuilder sRedirFile = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder(inout_sFilePath);
  sRedirFile.AppendPath("LocalCheckout.txt");

  // read redirection file, if available
  {
    WOSFile file;
    if (file.Open(sRedirFile, WFileOpenMode::Read).Succeeded())
    {
      WDataBuffer content;
      file.ReadAll(content);

      const WStringView sContent((const char*)content.GetData(), content.GetCount());

      if (sContent.EndsWith_NoCase("WProject") && WOSFile::ExistsFile(sContent))
      {
        inout_sFilePath = sContent;
        return WStatus(W_SUCCESS);
      }
    }
  }

  WString sName;
  WString sType;
  WString sUrl;
  WString sProjectFile;

  // read the info about the remote project from the OpenDDL config file
  {
    WOSFile file;
    if (file.Open(inout_sFilePath, WFileOpenMode::Read).Failed())
    {
      return WStatus(WFmt("Remote project file '{}' doesn't exist.", inout_sFilePath));
    }

    WDataBuffer content;
    file.ReadAll(content);

    WMemoryStreamContainerWrapperStorage<WDataBuffer> storage(&content);
    WMemoryStreamReader reader(&storage);

    WOpenDdlReader ddl;
    if (ddl.ParseDocument(reader).Failed())
    {
      return WStatus("Error in remote project DDL config file");
    }

    if (auto pRoot = ddl.GetRootElement())
    {
      if (auto pProject = pRoot->FindChildOfType("RemoteProject"))
      {
        if (auto pName = pProject->FindChildOfType(WOpenDdlPrimitiveType::String, "Name"))
        {
          sName = pName->GetPrimitivesString()[0];
        }
        if (auto pType = pProject->FindChildOfType(WOpenDdlPrimitiveType::String, "Type"))
        {
          sType = pType->GetPrimitivesString()[0];
        }
        if (auto pUrl = pProject->FindChildOfType(WOpenDdlPrimitiveType::String, "Url"))
        {
          sUrl = pUrl->GetPrimitivesString()[0];
        }
        if (auto pProjectFile = pProject->FindChildOfType(WOpenDdlPrimitiveType::String, "ProjectFile"))
        {
          sProjectFile = pProjectFile->GetPrimitivesString()[0];
        }
      }
    }
  }

  if (sType.IsEmpty() || sName.IsEmpty())
  {
    return WStatus(WFmt("Remote project '{}' DDL configuration is invalid.", inout_sFilePath));
  }

  WStringBuilder sTargetDir = opt_RemoteProjectDir.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (sTargetDir.IsEmpty())
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("This is a 'remote' project, meaning the data is not yet available on your machine.\n\nPlease select a folder where the project should be downloaded to.");

    static QString sPreviousFolder = WOSFile::GetUserDocumentsFolder().GetData();

    // native window, so not covered by WQtDialog - without '-remoteProjectDir' the target folder
    // has to come from a user
    if (WQtUiServices::SuppressModalWindow("Remote project download folder (file picker)"))
      return WStatus("A remote project needs a download folder. Pass '-remoteProjectDir' to give one without a user present.");

    QString sSelectedDir = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

    if (sSelectedDir.isEmpty())
    {
      return WStatus("");
    }

    sPreviousFolder = sSelectedDir;
    sTargetDir = sSelectedDir.toUtf8().data();
  }
  else
  {
    sTargetDir.MakeCleanPath();

    if (WOSFile::CreateDirectoryStructure(sTargetDir).Failed())
    {
      return WStatus(WFmt("Could not create the download directory '{}'.", sTargetDir));
    }
  }

  // if it is a git repository, clone it
  if (sType == "git" && !sUrl.IsEmpty())
  {
    {
      QStringList args;
      args << "clone";
      args << "--progress"; // it appears that this flag forces non-buffered I/O so it flushes output immediately after each write
      args << WMakeQString(sUrl);
      args << WMakeQString(sName);

      WProgressRange cloneProgress("Downloading Project", true);

      WProcessOptions po;
      po.m_sWorkingDirectory = sTargetDir.GetData();
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
      po.m_sProcess = "git.exe";
#else
      po.m_sProcess = "git";
#endif
      po.AddArgument("clone");
      po.AddArgument("--progress"); // it appears that this flag forces non-buffered I/O so it flushes output immediately after each write
      po.AddArgument(sUrl);
      po.AddArgument(sName);

      WProcessGroup pgroup;

      WString s_last_stderr_line;

      WDeque<WString> strings = {};
      WMutex mutex;

      // this function called on a separate thread
      po.m_onStdError = [&](WStringView out)
      {
        W_LOCK(mutex);
        strings.PushBack(out);
      };

      QApplication::setOverrideCursor(Qt::WaitCursor);
      W_SCOPE_EXIT(QApplication::restoreOverrideCursor());

      WResult res = pgroup.Launch(po);
      if (res.Failed())
      {
        pgroup.TerminateAll().IgnoreResult();
        return WStatus(WFmt("Running 'git' to download the remote project failed."));
      }

      // Without a user there is no progress bar to cancel, and its 'was canceled' state would end the
      // clone right away, so just wait for git to finish.
      const bool bHeadless = WQtUiServices::IsHeadless();

      while (!bHeadless)
      {
        // Process stderr output from git to update the progress bar.
        if (strings.GetCount() > 0)
        {
          W_LOCK(mutex);
          for (const auto& line : strings)
          {
            WString data = line;
            WStringBuilder str = data.GetData();

            if (const char* szPercent = str.FindLastSubString("%"))
            {
              str.SetSubString_FromTo(szPercent - 3, szPercent);
              str.Trim();

              WInt32 p;
              if (WConversionUtils::StringToInt(str, p).Succeeded())
              {
                double f_completion = p / 100.0;
                if (f_completion < cloneProgress.GetProgressbar()->GetCompletion())
                {
                  cloneProgress.GetProgressbar()->Reset();
                }
                cloneProgress.SetCompletion(f_completion);
              }
            }
            s_last_stderr_line = line;
          }
          strings.Clear();
        }

        if (cloneProgress.WasCanceled())
        {
          pgroup.TerminateAll().IgnoreResult();
          break;
        }

        qApp->processEvents();

        if (pgroup.GetProcesses()[0].GetState() == WProcessState::Finished)
          break;
      }

      res = pgroup.WaitToFinish();

      if (!bHeadless && cloneProgress.WasCanceled())
      {
        return WStatus("Project downloading cancelled by user. Please remove the incomplete project directory manually.");
      }
      else if (pgroup.GetProcesses()[0].GetExitCode() != 0)
      {
        return WStatus(WFmt("Failed to git clone the remote project '{}' from '{}'\n{}", sName, sUrl, s_last_stderr_line));
      }

      WLog::Success("Cloned remote project '{}' from '{}' to '{}'", sName, sUrl, sTargetDir);
    }

    // Find and extract any 7z archives in the cloned directory
    {
      WStringBuilder sClonedDir;
      sClonedDir.SetFormat("{}/{}", sTargetDir, sName);

      W_SUCCEED_OR_RETURN(ExtractArchivesInDirectory(sClonedDir));
    }

    inout_sFilePath.SetFormat("{}/{}/{}", sTargetDir, sName, sProjectFile);

    // write redirection file
    {
      WOSFile file;
      if (file.Open(sRedirFile, WFileOpenMode::Write).Succeeded())
      {
        file.Write(inout_sFilePath.GetData(), inout_sFilePath.GetElementCount()).AssertSuccess();
      }
    }

    return W_SUCCESS;
  }

  return WStatus(WFmt("Unknown remote project type '{}' or invalid URL '{}'", sType, sUrl));
}

bool WQtEditorApp::ExistsPluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  WStringBuilder path = szProjectDir;
  path.MakeCleanPath();

  if (path.EndsWith_NoCase("/WProject"))
    path.PathParentDirectory();

  path.AppendPath("Editor/PluginSelection.ddl");

  return WOSFile::ExistsFile(path);
}

void WQtEditorApp::WritePluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  WStringBuilder path = szProjectDir;
  path.AppendPath("Editor/PluginSelection.ddl");

  WFileWriter file;
  file.Open(path).AssertSuccess();

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  m_PluginBundles.WriteStateToDDL(ddl);
}

void WQtEditorApp::CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  WStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();

  for (auto it : m_PluginBundles.m_Plugins)
  {
    WPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplate);
  }

  WritePluginSelectionStateDDL(sPath);
}

void WQtEditorApp::LoadPluginBundleDlls(const char* szProjectFile)
{
  W_PROFILE_SCOPE("LoadPluginBundleDlls");
  WStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();
  sPath.AppendPath("Editor/PluginSelection.ddl");

  WFileReader file;
  if (file.Open(sPath).Succeeded())
  {
    WOpenDdlReader ddl;
    if (ddl.ParseDocument(file).Failed())
    {
      WLog::Error("Syntax error in plugin bundle file '{}'", sPath);
    }
    else
    {
      auto pState = ddl.GetRootElement()->FindChildOfType("PluginState");
      while (pState)
      {
        if (auto pName = pState->FindChildOfType(WOpenDdlPrimitiveType::String, "ID"))
        {
          const WString sID = pName->GetPrimitivesString()[0];

          bool bExisted = false;
          auto itPlug = m_PluginBundles.m_Plugins.FindOrAdd(sID, &bExisted);

          if (!bExisted)
          {
            WPluginBundle& bundle = itPlug.Value();
            bundle.m_bMissing = true;
            bundle.m_sDisplayName = sID;
            bundle.m_sDescription = "This plugin bundle is referenced by the project, but doesn't exist. Please check that all plugins are built correctly and their respective *.WPluginBundle files are copied to the binary directory.";
            bundle.m_LastModificationTime = WTimestamp::CurrentTimestamp();
          }
        }

        pState = pState->GetSibling();
      }

      m_PluginBundles.ReadStateFromDDL(ddl);
    }
  }

  WDynamicArray<WApplicationPluginConfig::PluginConfig> order;

  // first all the mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (!it.Value().m_bMandatory)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      WQtUiServices::MessageBoxWarning("The mandatory plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the WPluginBundle files correctly reference each other.");

      return;
    }
  }

  // now the non-mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || !it.Value().m_bSelected)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      WQtUiServices::MessageBoxWarning("The plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the WPluginBundle files correctly reference each other.");

      return;
    }
  }

  WSet<WString> NotLoaded;
  for (const WApplicationPluginConfig::PluginConfig& it : order)
  {
    W_PROFILE_SCOPE(it.m_sAppDirRelativePath.GetData());
    if (WPlugin::LoadPlugin(it.m_sAppDirRelativePath, it.m_bLoadCopy ? WPluginLoadFlags::LoadCopy : WPluginLoadFlags::Default).Failed())
    {
      NotLoaded.Insert(it.m_sAppDirRelativePath);
    }
  }

  if (!NotLoaded.IsEmpty())
  {
    WStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendFormat(" '{0}' \n", it.Key());
    }

    WQtUiServices::MessageBoxWarning(s);
  }
}

void WQtEditorApp::LaunchEditor(const char* szProject, bool bCreate)
{
  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    WStringBuilder sTemp = WOSFile::GetTempDataFolder("WEditor");
    sTemp.AppendPath("WEditorCrashIndicator");
    WOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  WStringBuilder app;
  app = WOSFile::GetApplicationDirectory();
  app.AppendPath("WEditor");
#if W_ENABLED(W_PLATFORM_WINDOWS)
  app.Append(".exe");
#endif
  app.MakeCleanPath();

  // TODO: pass through all command line arguments ?

  QStringList args;
  args << "-nosplash";
  args << (bCreate ? "-newproject" : "-project");
  args << QString::fromUtf8(szProject);

  if (m_StartupFlags.IsSet(StartupFlags::SafeMode))
    args << "-safe";
  if (m_StartupFlags.IsSet(StartupFlags::Dashboard))
    args << "-dashboard";

  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-renderer"))
  {
    WStringBuilder sRenderer = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    args << "-renderer";
    args << sRenderer.GetData();
  }

  QProcess proc;
  proc.startDetached(QString::fromUtf8(app, app.GetElementCount()), args);
}

const WApplicationPluginConfig WQtEditorApp::GetRuntimePluginConfig(bool bIncludeEditorPlugins) const
{
  WApplicationPluginConfig cfg;

  WTempHybridArray<WString, 16> order;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || it.Value().m_bSelected)
    {
      AddBundlesInOrder(cfg.m_Plugins, m_PluginBundles, it.Key(), false, bIncludeEditorPlugins, true).IgnoreResult();
    }
  }

  return cfg;
}

void WQtEditorApp::ReloadEngineResources()
{
  WSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  msg.m_sPayload = "ReloadAllResources";
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtEditorApp::OpenDemoDocument()
{
  auto* pCurator = WAssetCurator::GetSingleton();
  auto assets = pCurator->GetKnownAssets();

  WStringBuilder sBestDoc;

  for (auto it = assets->GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->m_Path.GetDataDirRelativePath().GetFileName().IsEqual_NoCase("Main"))
    {
      sBestDoc = it.Value()->m_Path.GetAbsolutePath();
      break;
    }
  }

  if (!sBestDoc.IsEmpty())
  {
    SlotQueuedOpenDocument(sBestDoc.GetData(), nullptr);
  }
}

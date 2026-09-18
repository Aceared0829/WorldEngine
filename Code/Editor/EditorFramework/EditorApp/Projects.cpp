#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/System/Window.h>
#include <Core/World/Component.h>
#include <EditorFramework/Actions/WindowLayoutActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

void UpdateInputDynamicEnumValues();

void WQtEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void WQtEditorApp::SlotQueuedCloseProject()
{
  // purge the image loading queue when a project is closed, but keep the existing cache
  WQtImageCache::GetSingleton()->StopRequestProcessing(false);

  WToolsProject::CloseProject();

  // enable image loading again, the queue is purged now
  WQtImageCache::GetSingleton()->EnableRequestProcessing();
}

WResult WQtEditorApp::OpenProject(const char* szProject, bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return CreateOrOpenProject(false, szProject);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szProject));
    return W_SUCCESS;
  }
}

void WQtEditorApp::SlotQueuedOpenProject(QString sProject)
{
  // Don't try to execute a queued project open if we have already shutdown the editor.
  if (m_bIsRunning)
    CreateOrOpenProject(false, sProject.toUtf8().data()).IgnoreResult();
}

WResult WQtEditorApp::CreateOrOpenProject(bool bCreate, WStringView sFile0)
{
  W_PROFILE_SCOPE("CreateOrOpenProject");
  WStringBuilder sFile = sFile0;
  if (!bCreate)
  {
    const WStatus status = MakeRemoteProjectLocal(sFile);
    if (status.Failed())
    {
      // if the message is empty, the user decided not to continue, so don't show an error message in this case
      if (!status.GetMessageString().IsEmpty())
      {
        WQtUiServices::GetSingleton()->MessageBoxStatus(status, "Opening remote project failed.");
      }

      return W_FAILURE;
    }
  }

  // check that we don't attempt to open a project from a different repository, due to code changes this often doesn't work too well
  if (!IsInHeadlessMode() && !m_bAnyProjectOpened)
  {
    WStringBuilder sdkDirFromProject;
    if (WFileSystem::FindFolderWithSubPath(sdkDirFromProject, sFile, "Data/Base", "WSdkRoot.txt").Succeeded())
    {
      sdkDirFromProject.MakeCleanPath();
      sdkDirFromProject.Trim(nullptr, "/");

      WStringView sdkDir = WFileSystem::GetSdkRootDirectory();
      sdkDir.Trim(nullptr, "/");

      // compare without case, because on Windows those can differ only in the drive letter's case ('d:/...' vs 'D:/...')
      if (!sdkDirFromProject.IsEqual_NoCase(sdkDir))
      {
        if (WQtUiServices::MessageBoxQuestion(WFmt("You are attempting to open a project that's located in a different SDK directory.\n\nSDK location: '{}'\nProject path: '{}'\n\nThis may make problems.\n\nContinue anyway?", sdkDir, sFile), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
        {
          return W_FAILURE;
        }
      }
    }
  }


  m_bLoadingProjectInProgress = true;
  W_SCOPE_EXIT(m_bLoadingProjectInProgress = false;);

  CloseSplashScreen();

  WStringBuilder sProjectFile = sFile;
  sProjectFile.MakeCleanPath();

  if (bCreate == false && !sProjectFile.EndsWith_NoCase("/WProject"))
  {
    sProjectFile.AppendPath("WProject");
  }

  if (WToolsProject::IsProjectOpen() && WToolsProject::GetSingleton()->GetProjectFile() == sProjectFile)
  {
    WQtUiServices::MessageBoxInformation("The selected project is already open");
    return W_FAILURE;
  }

  if (!WToolsProject::CanCloseProject())
    return W_FAILURE;

  WToolsProject::CloseProject();

  // create default plugin selection
  if (!ExistsPluginSelectionStateDDL(sProjectFile))
    CreatePluginSelectionDDL(sProjectFile, "General3D");

  WStatus res(W_SUCCESS);

  if (bCreate)
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, true);

      QApplication::closeAllWindows();
      return W_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      LoadPluginBundleDlls(sProjectFile);

      res = WToolsProject::CreateProject(sProjectFile);
    }
  }
  else
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, false);

      QApplication::closeAllWindows();
      return W_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      if (!IsInUnattendedMode())
      {
        WStringBuilder sTemp = WOSFile::GetTempDataFolder("WEditor");
        sTemp.AppendPath("WEditorCrashIndicator");
        WOSFile f;
        if (f.Open(sTemp, WFileOpenMode::Write, WFileShareMode::Exclusive).Succeeded())
        {
          f.Write(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
          f.Close();
          m_bWroteCrashIndicatorFile = true;
        }
      }

      {
        WStringBuilder sProjectDir = sProjectFile;
        sProjectDir.PathParentDirectory();

        WStringBuilder sSettingsFile = sProjectDir;
        sSettingsFile.AppendPath("Editor/CppProject.ddl");

        // first attempt to load project specific plugin bundles
        WCppSettings cppSettings;
        if (cppSettings.Load(sSettingsFile).Succeeded())
        {
          WQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(WCppProject::GetPluginSourceDir(cppSettings, sProjectDir));
        }

        // now load the plugin DLLs
        LoadPluginBundleDlls(sProjectFile);
      }

      res = WToolsProject::OpenProject(sProjectFile);
    }
  }

  if (res.Failed())
  {
    WStringBuilder s;
    s.SetFormat("Failed to open project:\n'{0}'", sProjectFile);

    WQtUiServices::MessageBoxStatus(res, s);
    return W_FAILURE;
  }


  if (m_StartupFlags.AreNoneSet(StartupFlags::SafeMode | StartupFlags::Headless))
  {
    WStringBuilder sAbsPath;

    if (!m_DocumentsToOpen.IsEmpty())
    {
      for (const auto& doc : m_DocumentsToOpen)
      {
        sAbsPath = doc;

        if (MakeDataDirectoryRelativePathAbsolute(sAbsPath))
        {
          SlotQueuedOpenDocument(sAbsPath.GetData(), nullptr);
        }
        else
        {
          WLog::Error("Document '{}' does not exist in this project.", doc);
        }
      }

      // don't try to open the same documents when the user switches to another project
      m_DocumentsToOpen.Clear();
    }
    else if (!m_StartupFlags.IsSet(StartupFlags::Dashboard))
    {
      const WRecentFilesList allDocs = LoadOpenDocumentsList();

      // Unfortunately this crashes in Qt due to the processEvents in the QtProgressBar
      // WProgressRange range("Restoring Documents", allDocs.GetFileList().GetCount(), true);

      for (auto& doc : allDocs.GetFileList())
      {
        // if (range.WasCanceled())
        //    break;

        // range.BeginNextStep(doc.m_File);
        SlotQueuedOpenDocument(doc.m_File.GetData(), nullptr);
      }

      if (allDocs.GetFileList().IsEmpty())
      {
        OpenDemoDocument();
      }
    }

    // Show the window maximized when opening a project
    WQtContainerWindow::GetContainerWindow()->showMaximized();
  }
  return W_SUCCESS;
}

void WQtEditorApp::ProjectEventHandler(const WToolsProjectEvent& r)
{
  switch (r.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectCreated:
      SetupNewProject();
      m_bSavePreferencesAfterOpenProject = true;
      break;

    case WToolsProjectEvent::Type::ProjectOpened:
    {
      W_PROFILE_SCOPE("ProjectOpened");
      WDynamicStringEnum::s_RequestUnknownCallback = WMakeDelegate(&WQtEditorApp::OnDemandDynamicStringEnumLoad, this);
      LoadProjectPreferences();
      SetupDataDirectories();
      ReadTagRegistry();
      UpdateInputDynamicEnumValues();

      // add project specific translations
      // (these are currently never removed)
      {
        WStringBuilder sProjectLocalization;
        sProjectLocalization.SetFormat(":project/Editor/Localization/{0}", WEditorPreferencesUser::GetLocalizationFolder(m_sActiveLanguage));
        m_pTranslatorFromFiles->AddTranslationFilesFromFolder(sProjectLocalization);
      }

      LogMissingComponentDocumentation();

      // tell the engine process which file system and plugin configuration to use
      WEditorEngineProcessConnection::GetSingleton()->SetFileSystemConfig(m_FileSystemConfig);
      WEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));

      WAssetCurator::GetSingleton()->StartInitialize(m_FileSystemConfig);
      if (WEditorEngineProcessConnection::GetSingleton()->RestartProcess().Failed())
      {
        W_PROFILE_SCOPE("ErrorLog");
        WLog::Error("Failed to start the engine process. Project loading incomplete.");
      }
      WAssetCurator::GetSingleton()->WaitForInitialize();

      m_sLastDocumentFolder = WToolsProject::GetSingleton()->GetProjectFile();
      m_sLastProjectFolder = WToolsProject::GetSingleton()->GetProjectFile();

      m_RecentProjects.Insert(WToolsProject::GetSingleton()->GetProjectFile(), 0);

      WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

      // Make sure preferences are saved, this is important when the project was just created.
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      else
      {
        // Save recent project list on project open in case of crashes or stopping the debugger.
        SaveRecentFiles();
      }

      if (m_StartupFlags.AreNoneSet(WQtEditorApp::StartupFlags::Headless | WQtEditorApp::StartupFlags::SafeMode | WQtEditorApp::StartupFlags::UnitTest | WQtEditorApp::StartupFlags::Background | WQtEditorApp::StartupFlags::Unattended))
      {
        if (WCppProject::ExistsProjectCMakeListsTxt())
        {
          WStatus compilerStatus = WCppProject::TestCompiler();
          if (compilerStatus.Failed())
          {
            WQtUiServices::MessageBoxWarning(WFmt("<html>The compiler preferences are invalid.<br><br>\
              This project has <a href='https://ezengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code.<br><br>\
              The compiler set in the preferences does not appear to work, as a result the plugin cannot be compiled <br><br><b>Error:</b> {}</html>",
              compilerStatus.GetMessageString()));
            break;
          }
          else if (WCppProject::IsBuildRequired())
          {
            const auto clicked = WQtUiServices::MessageBoxQuestion("<html>Compile this project's C++ plugin?<br><br>\
Explanation: This project has <a href='https://ezengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code. The plugin is currently not compiled and therefore the project won't fully work and certain assets will fail to transform.<br><br>\
It is advised to compile the plugin now, but you can also do so later.</html>",
              QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply, QMessageBox::StandardButton::Apply);

            if (clicked == QMessageBox::StandardButton::Ignore)
              break;

            QTimer::singleShot(1000, this, [this]()
              { WCppProject::EnsureCppPluginReady().IgnoreResult(); });
          }
        }


        WTimestamp lastTransform = WAssetCurator::GetSingleton()->GetLastFullTransformDate().GetTimestamp();

        if (pPreferences->m_bBackgroundAssetProcessing)
        {
          QTimer::singleShot(2000, this, [this]()
            { WAssetProcessor::GetSingleton()->StartProcessor(); });
        }
        else if (!lastTransform.IsValid() || (WTimestamp::CurrentTimestamp() - lastTransform).GetHours() > 5 * 24)
        {
          const auto clicked = WQtUiServices::MessageBoxQuestion("<html>Apply asset transformation now?<br><br>\
Explanation: For assets to work properly, they must be <a href='https://ezengine.net/pages/docs/assets/assets-overview.html#asset-transform'>transformed</a>. Otherwise they don't function as they should or don't even show up.<br>You can manually run the asset transform from the <a href='https://ezengine.net/pages/docs/assets/asset-browser.html#transform-assets'>asset browser</a> at any time.</html>",
            QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply, QMessageBox::StandardButton::Apply);

          if (clicked == QMessageBox::StandardButton::Ignore)
          {
            WAssetCurator::GetSingleton()->StoreFullTransformDate();
            break;
          }

          // check whether the project needs to be transformed
          QTimer::singleShot(2000, this, [this]()
            { WAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult(); });
        }
      }

      break;
    }

    case WToolsProjectEvent::Type::ProjectSaveState:
    {
      m_RecentProjects.Insert(WToolsProject::GetSingleton()->GetProjectFile(), 0);
      SaveSettings();

      // Auto-save the global editor layout before documents are closed
      if (!IsInHeadlessMode() && !IsInSafeMode() && !WToolsProject::GetSingleton()->IsProjectClosing())
      {
        WWindowLayoutActions::SaveUserLayout();
      }
      break;
    }

    case WToolsProjectEvent::Type::ProjectClosing:
    {
      WShutdownProcessMsgToEngine msg;
      WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      break;
    }

    case WToolsProjectEvent::Type::ProjectClosed:
    {
      WEditorEngineProcessConnection::GetSingleton()->ShutdownProcess();

      WAssetCurator::GetSingleton()->Deinitialize();

      // remove all data directories that were loaded by the project configuration
      WApplicationFileSystemConfig::Clear();
      WFileSystem::SetSpecialDirectory("project", nullptr); // removes this directory

      m_ReloadProjectRequiredReasons.Clear();
      UpdateGlobalStatusBarMessage();

      WPreferences::ClearProjectPreferences();

      // remove all dynamic enums that were dynamically loaded from the project directory
      {
        for (const auto& val : m_DynamicEnumStringsToClear)
        {
          WDynamicStringEnum::RemoveEnum(val);
        }
        m_DynamicEnumStringsToClear.Clear();
      }

      break;
    }

    case WToolsProjectEvent::Type::SaveAll:
    {
      WToolsProject::SaveProjectState();
      SaveAllOpenDocuments();
      break;
    }

    default:
      break;
  }
}

void WQtEditorApp::ProjectRequestHandler(WToolsProjectRequest& r)
{
  switch (r.m_Type)
  {
    case WToolsProjectRequest::Type::CanCloseProject:
    case WToolsProjectRequest::Type::CanCloseDocuments:
    {
      if (r.m_bCanClose == false)
        return;

      WTempHybridArray<WDocument*, 32> ModifiedDocs;
      if (r.m_Type == WToolsProjectRequest::Type::CanCloseProject)
      {
        for (WDocumentManager* pMan : WDocumentManager::GetAllDocumentManagers())
        {
          for (WDocument* pDoc : pMan->GetAllOpenDocuments())
          {
            if (!pDoc->IsModified())
              continue;

            // Only ask about documents the user can actually see.
            // A document without a window was opened programmatically.
            if (!pDoc->HasWindowBeenRequested())
            {
              WLog::Info("Discarding unsaved changes in '{}', which is open without a window.", pDoc->GetDocumentPath());
              continue;
            }

            ModifiedDocs.PushBack(pDoc);
          }
        }
      }
      else
      {
        for (WDocument* pDoc : r.m_Documents)
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        WQtModifiedDocumentsDlg dlg(QApplication::activeWindow(), ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bCanClose = false;
      }
    }
    break;
    case WToolsProjectRequest::Type::SuggestContainerWindow:
    {
      const auto& docs = GetRecentDocumentsList();
      WStringBuilder sCleanPath = r.m_Documents[0]->GetDocumentPath();
      sCleanPath.MakeCleanPath();

      for (auto& file : docs.GetFileList())
      {
        if (file.m_File == sCleanPath)
        {
          r.m_iContainerWindowUniqueIdentifier = file.m_iContainerWindow;
          break;
        }
      }
    }
    break;
    case WToolsProjectRequest::Type::GetPathForDocumentGuid:
    {
      if (WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(r.m_documentGuid))
      {
        r.m_sAbsDocumentPath = pSubAsset->m_pAssetInfo->m_Path;
      }
    }
    break;
  }
}

void WQtEditorApp::SetupNewProject()
{
  WToolsProject::GetSingleton()->CreateSubFolder("Editor");
  WToolsProject::GetSingleton()->CreateSubFolder("RuntimeConfigs");

  // write the default window config
  {
    WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    if (!WFileSystem::ExistsFile(sPath))
    {
      WWindowCreationDesc desc;
      desc.m_Title = WToolsProject::GetSingleton()->GetProjectName(false);
      desc.SaveToDDL(sPath).IgnoreResult();
    }
  }

  // write a stub input mapping
  {
    WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

    if (!WFileSystem::ExistsFile(sPath))
    {
      WDeferredFileWriter file;
      file.SetOutput(sPath);

      WTempHybridArray<WGameAppInputConfig, 4> actions;
      WGameAppInputConfig& a = actions.ExpandAndGetRef();
      a.m_sInputSet = "Default";
      a.m_sInputAction = "Interact";
      a.m_bApplyTimeScaling = false;
      a.m_sInputSlotTrigger[0] = WInputSlot_KeySpace;
      a.m_sInputSlotTrigger[1] = WInputSlot_MouseButton0;
      a.m_sInputSlotTrigger[2] = WInputSlot_Controller0_ButtonA;

      WGameAppInputConfig::WriteToDDL(file, actions);

      file.Close().IgnoreResult();
    }
  }
}

void WQtEditorApp::LogMissingComponentDocumentation()
{
  W_LOG_BLOCK("Missing Component Documentation");

  WRTTI::ForEachDerivedType<WComponent>(
    [](const WRTTI* pRtti)
    {
      if (pRtti->GetAttributeByType<WInDevelopmentAttribute>() != nullptr)
        return;
      if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr)
        return;

      WStringView sTypeName = pRtti->GetTypeName();
      if (WTranslateHelpURL(sTypeName).IsEmpty())
      {
        WLog::Warning("Component '{}' has no documentation link.", sTypeName);
      }
    },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

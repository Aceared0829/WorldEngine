#include <EditorProcessor/EditorProcessorPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Application.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

WCommandLineOptionPath opt_OutputDir("_EditorProcessor", "-outputDir", "Output directory", "");
WCommandLineOptionBool opt_SaveProfilingData("_EditorProcessor", "-profiling", "Saves performance profiling information into the output folder.", false);
WCommandLineOptionPath opt_Project("_EditorProcessor", "-project", "Path to the project folder.", "");
WCommandLineOptionBool opt_Resave("_EditorProcessor", "-resave", "If specified, assets will be resaved.", false);
WCommandLineOptionString opt_Transform("_EditorProcessor", "-transform", "If specified, assets will be transformed for the given platform profile.\n\
\n\
Example:\n\
  -transform Default\n\
",
  "");
WCommandLineOptionBool opt_Compile("_EditorProcessor", "-compile", "If specified, the C++ project will be generated and compiled.", false);
WCommandLineOptionBool opt_Recompile("_EditorProcessor", "-recompile", "Like -compile, but re-runs CMake and compiles unconditionally.\n\
\n\
Use this when files were added to or removed from the C++ source directory, which -compile does not notice.\n\
",
  false);
WCommandLineOptionBool opt_FailOnError("_EditorProcessor", "-failonerror",
  "Set the return code to 1 if any error was logged during the run.\n\
\n\
Without this, an operation that logged errors but still completed exits with 0, which a script cannot\n\
tell apart from a clean run.\n\
",
  false);
WCommandLineOptionPath opt_Export("_EditorProcessor", "-export", "If specified, the project is exported to the given (absolute) directory.\n\
\n\
The target directory is deleted first. The C++ project is built and all assets are transformed beforehand,\n\
the latter unless -transform already did so in the same run. An 'ExportLog.txt' is written into the target\n\
directory.\n\
\n\
Example:\n\
  -export \"C:/Temp/MyGame\"\n\
",
  "");

class WEditorProcessorApplication : public WApplication
{
public:
  using SUPER = WApplication;

  WEditorProcessorApplication()
    : WApplication("WEditor")
  {
    EnableMemoryLeakReporting(true);
    m_pEditorEngineProcessAppDummy = W_DEFAULT_NEW(WEditorEngineProcessApp);

    m_pEditorApp = new WQtEditorApp;
  }

  virtual WResult BeforeCoreSystemsStartup() override
  {
    WStartup::AddApplicationTag("tool");
    WStartup::AddApplicationTag("editor");
    WStartup::AddApplicationTag("editorprocessor");

    WQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    WString sUserDataFolder = WApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    WString sOutputFolder = opt_OutputDir.GetOptionValue(WCommandLineOption::LogMode::Never);
    WCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(sOutputFolder.IsEmpty() ? sUserDataFolder : sOutputFolder, "EditorProcessor");
    WCrashHandler::SetCrashHandler(&WCrashHandler_WriteMiniDump::g_Instance);

    m_bFailOnError = opt_FailOnError.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

    if (m_bFailOnError)
    {
      m_LogErrorCounterID = WGlobalLog::AddLogWriter(WMakeDelegate(&WEditorProcessorApplication::OnLogEvent, this));
    }

    return W_SUCCESS;
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bFailOnError && m_iLoggedErrors > 0)
    {
      WLog::Info("{} errors were logged.", (WInt32)m_iLoggedErrors);

      // an explicitly reported failure is more specific than 'something was logged', so it wins
      if (GetReturnCode() == 0)
      {
        SetReturnCode(1);
      }
    }

    SUPER::BeforeCoreSystemsShutdown();

    if (m_LogErrorCounterID != 0)
    {
      WGlobalLog::RemoveLogWriter(m_LogErrorCounterID);
      m_LogErrorCounterID = 0;
    }
  }

  void OnLogEvent(const WLoggingEventData& e)
  {
    if (e.m_EventType == WLogMsgType::ErrorMsg || e.m_EventType == WLogMsgType::SeriousWarningMsg)
    {
      m_iLoggedErrors.Increment();
    }
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    m_pEditorEngineProcessAppDummy = nullptr;

    WQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  void EventHandlerIPC(const WProcessCommunicationChannel::Event& e)
  {
    if (const WProcessAssetMsg* pMsg = WDynamicCast<const WProcessAssetMsg*>(e.m_pMessage))
    {
      if (pMsg->m_sAssetPath.HasExtension("WPrefab") || pMsg->m_sAssetPath.HasExtension("WScene"))
      {
        WQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
      }

      WProcessAssetResponseMsg msg;
      msg.m_StartedProcessing = WTime::Now();
      {
        WLogEntryDelegate logger([&msg](WLogEntry& ref_entry) -> void
          { msg.m_LogEntries.PushBack(std::move(ref_entry)); },
          WLogMsgType::WarningMsg);
        WLogSystemScope logScope(&logger);

        const WUInt32 uiPlatform = WAssetCurator::GetSingleton()->FindAssetProfileByName(pMsg->m_sPlatform);

        if (uiPlatform == WInvalidIndex)
        {
          WLog::Error("Asset platform config '{0}' is unknown", pMsg->m_sPlatform);
        }
        else
        {
          WUInt64 uiAssetHash = 0;
          WUInt64 uiThumbHash = 0;
          WUInt64 uiPackageHash = 0;

          // TODO: there is currently no 'nice' way to switch the active platform for the asset processors
          // it is also not clear whether this is actually safe to execute here
          WAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);
          // First, force checking for file system changes for the asset and the transitive hull of all dependencies and runtime references. This needs to be done as this EditorProcessor instance might not know all the files yet as some might just have been written. We can't rely on the filesystem watcher as it is not instant and also might just miss some events.
          for (const WString& sDepOrRef : pMsg->m_DepRefHull)
          {
            if (sDepOrRef.IsAbsolutePath())
            {
              WAssetCurator::GetSingleton()->NotifyOfFileChange(sDepOrRef);
            }
            else
            {
              WStringBuilder sTemp = sDepOrRef;
              if (WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
              {
                WAssetCurator::GetSingleton()->NotifyOfFileChange(sTemp);
              }
            }
          }
          WAssetCurator::GetSingleton()->NotifyOfFileChange(pMsg->m_sAssetPath);

          // Next, we force checking that the asset is up to date. This EditorProcessor instance might not have observed the generation of the output files of various dependencies yet and incorrectly assume that some dependencies still need to be transformed. To prevent this, we force checking the asset and all its dependencies via the filesystem, ignoring the caching.
          WAssetInfo::TransformState state = WAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, WAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, uiPackageHash, true);
          msg.m_StartedTransform = WTime::Now();

          if ((uiAssetHash != pMsg->m_AssetHash) || (uiThumbHash != pMsg->m_ThumbHash))
          {
            WLog::Warning("Asset '{}' of state '{}' in processor with hashes '{}|{}' differs from the state in the editor with hashes '{}|{}'", pMsg->m_sAssetPath, (int)state, uiAssetHash, uiThumbHash, pMsg->m_AssetHash, pMsg->m_ThumbHash);
            msg.m_uiMissmatchAssetHash = uiAssetHash;
            msg.m_uiMissmatchThumbHash = uiThumbHash;

            WSet<WString> dependencies;
            WAssetCurator::GetSingleton()->GenerateTransitiveHull(pMsg->m_sAssetPath, dependencies, WDependencyFlags::Transform);
            WAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, WDependencyFlags::Transform, msg.m_MissmatchTransformDependencies);

            dependencies.Clear();
            WAssetCurator::GetSingleton()->GenerateTransitiveHull(pMsg->m_sAssetPath, dependencies, WDependencyFlags::Thumbnail);
            WAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, WDependencyFlags::Thumbnail, msg.m_MissmatchThumbnailDependencies);
          }

          if (state == WAssetInfo::NeedsThumbnail || state == WAssetInfo::NeedsTransform)
          {
            msg.m_Status = WAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid, WTransformFlags::BackgroundProcessing, WAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));

            if (msg.m_Status.Failed())
            {
              // make sure the result message ends up in the log
              WLog::Error("{}", msg.m_Status.m_sMessage);
            }

            // As there is no game loop that would progress frames in the engine process as it only waits for messages, we have to forcefully destroy pending deletion in the GAL after each transform or the process might never free those resources if it doesn't get a thumbnail job to do which has to tick the render loop.
            WSimpleConfigMsgToEngine msg;
            msg.m_sWhatToDo = "FreeGalResources";
            WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
          }
          else if (state == WAssetInfo::UpToDate)
          {
            msg.m_Status = WTransformStatus();
            WLog::Warning("Asset already up to date: '{}'", pMsg->m_sAssetPath);
          }
          else
          {
            msg.m_Status = WTransformStatus(WFmt("Asset {} is in state {}, can't process asset.", pMsg->m_sAssetPath, (int)state)); // TODO nicer state to string
            WLog::Error("{}", msg.m_Status.m_sMessage);
          }
        }
      }
      msg.m_FinishedProcessing = WTime::Now();
      m_IPC.SendMessage(&msg);
    }
    else if (const WFreeAllResourcesMsg* pMsg = WDynamicCast<const WFreeAllResourcesMsg*>(e.m_pMessage))
    {
      // We have no more jobs for this processor so let's tell the engine process to free up resources.
      WSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "FreeAllResources";
      WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    }
  }

  virtual void Run() override
  {
    {
      WStringBuilder cmdHelp;
      // '_Editor' is included because the editor's own startup options - '-createProject' among them - are
      // handled by WQtEditorApp::StartupEditor(), which this application runs as well
      if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_EditorProcessor;_Editor;cvar"))
      {
        WQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        QuitApplication();
        return;
      }
    }

#if W_ENABLED(W_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
    const WString sTransformProfile = opt_Transform.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bRecompile = opt_Recompile.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bCompile = opt_Compile.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified) || bRecompile;
    const bool bResave = opt_Resave.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
    const WString sExportDir = opt_Export.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
    // '-createProject' belongs to the editor application, which handles it during StartupEditor() - it is only
    // read here to tell "this run has work to do" from "this run waits for IPC jobs"
    const bool bCreateProject = !WCommandLineUtils::GetGlobalInstance()->GetStringOption("-createProject").IsEmpty();
    const bool bBackgroundMode = sTransformProfile.IsEmpty() && !bResave && !bCompile && sExportDir.IsEmpty() && !bCreateProject;
    const WString sOutputDir = opt_OutputDir.GetOptionValue(WCommandLineOption::LogMode::Always);
    const WBitflags<WQtEditorApp::StartupFlags> startupFlags = bBackgroundMode ? WQtEditorApp::StartupFlags::Headless | WQtEditorApp::StartupFlags::Background : WQtEditorApp::StartupFlags::Headless;
    WQtEditorApp::GetSingleton()->StartupEditor(startupFlags, sOutputDir);
    WQtUiServices::SetHeadless(true);

    QCoreApplication::sendPostedEvents();
    qApp->processEvents();

    W_SCOPE_EXIT(WQtEditorApp::GetSingleton()->ShutdownEditor(); QuitApplication(););

    const WStringBuilder sProject = opt_Project.GetOptionValue(WCommandLineOption::LogMode::Always);

    // Project is opened by StartupEditor
    if (!sProject.IsEmpty() && !WToolsProject::IsProjectOpen())
    {
      WLog::Error("Failed to open project: {}", sProject);
      SetReturnCode(2);
      return;
    }

    // same for a newly created one - StartupEditor() creates and opens it, so a closed project means it failed
    if (bCreateProject && !WToolsProject::IsProjectOpen())
    {
      SetReturnCode(5);
      return;
    }

    if (!sTransformProfile.IsEmpty() || bCompile || !sExportDir.IsEmpty() || bCreateProject)
    {
      // before we transform any assets or if specifically asked, make sure the C++ code is built
      {
        WCppSettings cppSettings;
        if (cppSettings.Load().Succeeded())
        {
          // -recompile exists because BuildCodeIfNecessary() only runs CMake when the solution is missing
          // or its cache is outdated, which does not cover source files being added or removed.
          WResult res = W_SUCCESS;

          if (bRecompile)
          {
            res = WCppProject::RunCMake(cppSettings);

            if (res.Succeeded())
            {
              res = WCppProject::CompileSolution(cppSettings);
            }
          }
          else
          {
            res = WCppProject::BuildCodeIfNecessary(cppSettings);
          }

          if (res.Failed())
          {
            SetReturnCode(3);
            return;
          }

          WQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
        }
      }

      if (!sTransformProfile.IsEmpty() || !sExportDir.IsEmpty())
      {
        // Both steps need the editor to be idle, so they share one handler - and they have to run in this
        // order, because an export picks up whatever the last transform produced.
        bool bWorkPending = true;

        WQtEditorApp::GetSingleton()->connect(WQtEditorApp::GetSingleton(), &WQtEditorApp::IdleEvent, WQtEditorApp::GetSingleton(), [this, &bWorkPending, &sTransformProfile, &sExportDir]()
          {
          if (!bWorkPending)
            return;

          bWorkPending = false;

          bool bTransformed = false;

          if (!sTransformProfile.IsEmpty())
          {
            const WUInt32 uiPlatform = WAssetCurator::GetSingleton()->FindAssetProfileByName(sTransformProfile);

            if (uiPlatform == WInvalidIndex)
            {
              WLog::Error("Asset platform config '{0}' is unknown", sTransformProfile);
              SetReturnCode(1);
            }
            else
            {
              // so that a following export writes the assets of the profile that was just transformed,
              // rather than those of whatever profile the project happens to start up with
              WAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);

              WStatus status = WAssetCurator::GetSingleton()->TransformAllAssets(WTransformFlags::TriggeredManually, WAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));
              if (status.Failed())
              {
                status.LogFailure();
                SetReturnCode(1);
              }

              bTransformed = true;
            }

            if (opt_SaveProfilingData.GetOptionValue(WCommandLineOption::LogMode::Always))
            {
              WActionContext context;
              WActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
            }
          }

          if (!sExportDir.IsEmpty() && GetReturnCode() == 0)
          {
            WProjectExportOptions options;
            // both already done above: the C++ plugin by the block before the event loop, the assets by
            // the transform step, for the explicitly requested profile
            options.m_bCompileCppPlugin = false;
            options.m_bTransformAssets = !bTransformed;

            const WStatus status = WProjectExport::ExportProjectComplete(sExportDir, options);

            if (status.Failed())
            {
              status.LogFailure();
              SetReturnCode(4);
            }
            else
            {
              WLog::Success("Exported project to '{}'.", sExportDir);
            }
          }

          QApplication::quit(); });
      }
      else
      {
        WQtEditorApp::GetSingleton()->connect(WQtEditorApp::GetSingleton(), &WQtEditorApp::IdleEvent, WQtEditorApp::GetSingleton(), [this]()
          { QApplication::quit(); });
      }

      const WInt32 iReturnCode = WQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else if (opt_Resave.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified))
    {
      WQtEditorApp::GetSingleton()->connect(WQtEditorApp::GetSingleton(), &WQtEditorApp::IdleEvent, WQtEditorApp::GetSingleton(), [this]()
        {
        WAssetCurator::GetSingleton()->ResaveAllAssets("");

        if (opt_SaveProfilingData.GetOptionValue(WCommandLineOption::LogMode::Always))
        {
          WActionContext context;
          WActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }

        QApplication::quit(); });

      const WInt32 iReturnCode = WQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else
    {
      WResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(WMakeDelegate(&WEditorProcessorApplication::EventHandlerIPC, this));
        WQtEditorApp::GetSingleton()->connect(WQtEditorApp::GetSingleton(), &WQtEditorApp::IdleEvent, WQtEditorApp::GetSingleton(), [this]()
          {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.WaitForMessages();

          bRecursionBlock = false; });

        const WInt32 iReturnCode = WQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {
        WLog::Error("Failed to connect with host process");
        this->SetReturnCode(200);
      }
    }
  }

private:
  WQtEditorApp* m_pEditorApp;
  WEngineProcessCommunicationChannel m_IPC;
  WUniquePtr<WEditorEngineProcessApp> m_pEditorEngineProcessAppDummy;

  bool m_bFailOnError = false;
  // written from every thread that logs
  WAtomicInteger32 m_iLoggedErrors = 0;
  WEventSubscriptionID m_LogErrorCounterID = 0;
};

W_APPLICATION_ENTRY_POINT(WEditorProcessorApplication);

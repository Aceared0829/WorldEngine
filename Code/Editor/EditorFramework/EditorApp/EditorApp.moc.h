#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QApplication>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

class QMainWindow;
class QWidget;
class WProgress;
class WQtProgressbar;
class WQtEditorApp;
template <typename T>
class QList;
using QStringList = QList<QString>;
class WTranslatorFromFiles;
class WDynamicStringEnum;
class QSplashScreen;
class WQtVersionChecker;

struct W_EDITORFRAMEWORK_DLL WEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom
                                ///< dependencies at the right moment.
    ReloadResources,            ///< Sent when 'ReloadResources' has been triggered (and a message was sent to the engine)
    EditorStarted,              ///< Editor has finished all initialization code and will now load the recent project.
  };

  Type m_Type;
};

class W_EDITORFRAMEWORK_DLL WQtEditorApp : public QObject
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtEditorApp);

public:
  struct StartupFlags
  {
    using StorageType = WUInt8;
    enum Enum
    {
      Headless = W_BIT(0),   ///< The app does not do any rendering.
      SafeMode = W_BIT(1),   ///< '-safe' : Prevent automatic loading of projects, scenes, etc. to minimize risk of crashing.
      Dashboard = W_BIT(2),  ///< '-dashboard' : Don't restore the previous session, i.e. neither load the most recent project nor reopen its documents.
      UnitTest = W_BIT(3),   ///< Specified when the process is running as a unit test
      Background = W_BIT(4), ///< This process is an editor processor background process handling IPC tasks of the editor parent process.
      Unattended = W_BIT(5), ///< '-unattended' : No user is present, don't show anything that would block waiting for input.
      Default = 0,
    };

    struct Bits
    {
      StorageType Headless : 1;
      StorageType SafeMode : 1;
      StorageType Dashboard : 1;
      StorageType UnitTest : 1;
      StorageType Background : 1;
      StorageType Unattended : 1;
    };
  };

public:
  WQtEditorApp();
  ~WQtEditorApp();

  static WEvent<const WEditorAppEvent&> m_Events;

  //
  // External Tools
  //

  /// Searches for an external tool.
  ///
  /// Either uses one from the precompiled tools folder, or from the currently compiled binaries, depending where it finds one.
  /// If the editor preference is set to use precompiled tools, that folder is preferred, otherwise the other folder is preferred.
  WString FindToolApplication(const char* szToolName);

  /// Executes an external tool as found by FindToolApplication().
  ///
  /// The applications output is parsed and forwarded to the given log interface. A custom log level is applied first.
  /// If the tool cannot be found or it takes longer to execute than the allowed timeout, the function returns failure.
  WStatus ExecuteTool(const char* szTool, const QStringList& arguments, WUInt32 uiSecondsTillTimeout, WLogInterface* pLogOutput = nullptr, WLogMsgType::Enum logLevel = WLogMsgType::WarningMsg, const char* szCWD = nullptr);

  /// Creates the string with which to run Fileserve for the currently open project.
  WString BuildFileserveCommandLine() const;

  /// Launches Fileserve with the settings for the current project.
  void RunFileserve();

  /// Launches WInspector, connecting to the given port. Pass 0 to use the Inspector's default/last-used connection.
  void RunInspector(WUInt16 uiPort = 0);

  /// Launches Tracy.
  void RunTracy();

  //
  //
  //

  /// Returns whether we are between StartupEditor and ShutdownEditor.
  bool IsRunning() const { return m_bIsRunning; }

  /// Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_StartupFlags.IsSet(StartupFlags::SafeMode); }

  /// Returns true if the the app shouldn't display anything. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_StartupFlags.IsSet(StartupFlags::Headless); }

  /// Returns true if the editor is started in run in test mode.
  bool IsInUnitTestMode() const { return m_StartupFlags.IsSet(StartupFlags::UnitTest); }

  /// Returns true if the editor is started in run in background mode.
  bool IsBackgroundMode() const { return m_StartupFlags.IsSet(StartupFlags::Background); }

  /// Can be set via the command line option '-unattended'. In this mode no user is present to interact with the editor.
  ///
  /// The editor still shows its UI, but must not display anything modal, since nobody would ever close it and the
  /// editor would stall forever. Message boxes going through WQtUiServices already handle this, but anything that
  /// opens a dialog itself, or that waits for a user decision in some other way, has to check this flag.
  ///
  /// Headless mode implies this, there is no window to show a dialog on.
  bool IsInUnattendedMode() const { return m_StartupFlags.IsSet(StartupFlags::Unattended) || m_StartupFlags.IsSet(StartupFlags::Headless); }

  const WPluginBundleSet& GetPluginBundles() const { return m_PluginBundles; }
  WPluginBundleSet& GetPluginBundles() { return m_PluginBundles; }

  void AddRestartRequiredReason(const char* szReason);
  const WSet<WString>& GetRestartRequiredReasons() { return m_RestartRequiredReasons; }

  void AddReloadProjectRequiredReason(const char* szReason);
  const WSet<WString>& GetReloadProjectRequiredReason() { return m_ReloadProjectRequiredReasons; }

  void SaveSettings();

  /// Writes a file containing all the currently open documents
  void SaveOpenDocumentsList();

  /// Reads the list of last open documents in the current project.
  WRecentFilesList LoadOpenDocumentsList();

  void InitQt(int iArgc, char** pArgv);
  void StartupEditor();
  void StartupEditor(WBitflags<StartupFlags> startupFlags, const char* szUserDataFolder = nullptr);
  void ShutdownEditor();
  WInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();

  WRecentFilesList& GetRecentProjectsList() { return m_RecentProjects; }
  WRecentFilesList& GetRecentDocumentsList() { return m_RecentDocuments; }

  WEditorEngineProcessConnection* GetEngineViewProcess() { return m_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  void CloseProject();
  WResult OpenProject(const char* szProject, bool bImmediate = false);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiOpenDashboard();
  void GuiOpenDocsAndCommunity();
  bool GuiCreateProject(bool bImmediate = false);
  bool GuiOpenProject(bool bImmediate = false);

  void OpenDocumentQueued(WStringView sDocument, const WDocumentObject* pOpenContext = nullptr);
  WDocument* OpenDocument(WStringView sDocument, WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext = nullptr);
  WDocument* CreateDocument(WStringView sDocument, WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext = nullptr);

  WResult CreateOrOpenProject(bool bCreate, WStringView sFile);

  /// Creates a project as described by the '-createProject', '-projectTemplate' and '-pluginTemplate' command line options.
  ///
  /// Only creates the files, the caller opens the result. Failures are logged, since a command line caller has no other channel.
  WResult CreateProjectFromCommandLine(WStringView sTargetDirectory);

  /// Logs the project templates and plugin templates that '-createProject' accepts ('-listTemplates').
  void LogAvailableTemplates();

  /// If this project is remote, ie coming from another repository that is not checked-out by default, make sure it exists locally on disk.
  ///
  /// Adjusts inout_sFilePath from pointing to an WRemoteProject file to a WProject file, if necessary.
  /// If the project is already local, it always succeeds.
  /// If checking out fails or is user canceled, the function returns failure.
  WStatus MakeRemoteProjectLocal(WStringBuilder& inout_sFilePath);

  bool ExistsPluginSelectionStateDDL(const char* szProjectDir = ":project");
  void WritePluginSelectionStateDDL(const char* szProjectDir = ":project");
  void CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate);
  void LoadPluginBundleDlls(const char* szProjectFile);
  void DetectAvailablePluginBundles(WStringView sSearchDirectory);

  /// Launches a new instance of the editor to open the given project.
  void LaunchEditor(const char* szProject, bool bCreate);

  /// Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  /// If uiInsertIndex is not WInvalidIndex, a newly added directory is inserted at that position instead of appended.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false, WUInt32 uiInsertIndex = WInvalidIndex);

  const WApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }

  /// Collects the data directories of all bundles that are currently active (mandatory, selected, or a transitive
  /// requirement of one of those), normalized for comparison.
  void GetActiveBundleDataDirectories(WSet<WString>& out_dirs) const;
  const WApplicationPluginConfig GetRuntimePluginConfig(bool bIncludeEditorPlugins) const;

  void SetFileSystemConfig(const WApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(WStringBuilder& ref_sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(WString& ref_sPath) const;
  bool MakePathDataDirectoryRelative(WStringBuilder& ref_sPath) const;
  bool MakePathDataDirectoryRelative(WString& ref_sPath) const;

  bool MakePathDataDirectoryParentRelative(WStringBuilder& ref_sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(WStringBuilder& ref_sPath, bool bCheckExists) const;

  WStatus SaveTagRegistry();

  /// Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/WEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(WDynamicArray<WString>& slots) const;

  /// Instructs the engine to reload its resources
  void ReloadEngineResources();

  void RestartEngineProcessIfPluginsChanged(bool bForce);
  void SetStyleSheet();

Q_SIGNALS:
  void IdleEvent();

private:
  WString BuildDocumentTypeFileFilter(bool bForCreation);

  void GuiCreateOrOpenDocument(bool bCreate);
  bool GuiCreateOrOpenProject(bool bCreate);

private Q_SLOTS:
  void SlotTimedUpdate();
  void SlotAutoSave();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject, void* pOpenContext);
  void SlotQueuedGuiOpenDashboard();
  void SlotQueuedGuiOpenDocsAndCommunity();
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);
  void SlotSaveSettings();
  void SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced);

private:
  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(WDocumentManager::Request& r);
  void DocumentManagerEventHandler(const WDocumentManager::Event& r);
  void DocumentEventHandler(const WDocumentEvent& e);
  void ProjectRequestHandler(WToolsProjectRequest& r);
  void ProjectEventHandler(const WToolsProjectEvent& r);
  void EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e);
  void UiServicesEvents(const WQtUiServices::Event& e);

  /// Silences the assert dialog for the editor and every process it launches, for unattended mode.
  void SetupSilentAsserts();

  void SetupNewProject();
  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void LogMissingComponentDocumentation();
  void StoreEnginePluginModificationTimes();
  bool CheckForEnginePluginModifications();
  void SaveAllOpenDocuments();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void CreatePanels();

  void SetupAndShowSplashScreen();
  void CloseSplashScreen();

  void OpenDemoDocument();

  WResult AddBundlesInOrder(WDynamicArray<WApplicationPluginConfig::PluginConfig>& order, const WPluginBundleSet& bundles, const WString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const;

  /// Collects the data directories declared by every known bundle (active or not), normalized for comparison.
  /// Used to identify and prune stale bundle data directory entries.
  void GetAllKnownBundleDataDirectories(WSet<WString>& out_dirs) const;

  bool m_bSavePreferencesAfterOpenProject;
  bool m_bLoadingProjectInProgress = false;
  bool m_bAnyProjectOpened = false;
  bool m_bWroteCrashIndicatorFile = false;
  bool m_bIsRunning = false;

  WBitflags<StartupFlags> m_StartupFlags;
  WDynamicArray<WString> m_DocumentsToOpen;

  WSet<WString> m_RestartRequiredReasons;
  WSet<WString> m_ReloadProjectRequiredReasons;

  WPluginBundleSet m_PluginBundles;

  void SaveRecentFiles();
  void LoadRecentFiles();

  WRecentFilesList m_RecentProjects;
  WRecentFilesList m_RecentDocuments;

  int m_iArgc = 0;
  QApplication* m_pQtApplication = nullptr;
  WLongOpControllerManager m_LongOpControllerManager;
  WEditorEngineProcessConnection* m_pEngineViewProcess;
  QTimer* m_pTimer = nullptr;

  QSplashScreen* m_pSplashScreen = nullptr;
  QTimer* m_pAutoSaveTimer = nullptr;

  WLogWriter::HTML m_LogHTML;

  WTime m_LastPluginModificationCheck;
  WApplicationFileSystemConfig m_FileSystemConfig;

  // *** Recent Paths ***
  WString m_sLastDocumentFolder;
  WString m_sLastProjectFolder;

  // *** Progress Bar ***
public:
  bool IsProgressBarProcessingEvents() const;

private:
  WProgress* m_pProgressbar = nullptr;
  WQtProgressbar* m_pQtProgressbar = nullptr;

  // *** Localization ***
  WTranslatorFromFiles* m_pTranslatorFromFiles = nullptr;

  // Language code the UI was started with, e.g. "en" or "zh-CN". Resolved once during startup from
  // WEditorPreferencesUser and kept so that project specific translations can be loaded from the
  // matching sub folder.
  WString m_sActiveLanguage;

  // *** Dynamic Enum Strings ***
  WSet<WString> m_DynamicEnumStringsToClear;
  void OnDemandDynamicStringEnumLoad(WStringView sEnumName, WDynamicStringEnum& e);

  WUniquePtr<WQtVersionChecker> m_pVersionChecker;
};

W_DECLARE_FLAGS_OPERATORS(WQtEditorApp::StartupFlags);

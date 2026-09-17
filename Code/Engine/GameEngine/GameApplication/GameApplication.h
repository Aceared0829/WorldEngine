#pragma once

#include <GameEngine/GameState/GameState.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/TextFileWriter.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>

class WConsole;

/// Which input actions the WGameApplication may register and execute (see WGameApplication::RegisterGameApplicationInputActions())
struct WGameApplicationInputFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,
    All = 0xFFFFFFFF,

    LoadInputConfig = W_BIT(0),           ///< Whether to load the project's WGameAppInputConfig

    Dev_EscapeToClose = W_BIT(8),         ///< Register the ESC key to close the application without asking
    Dev_Console = W_BIT(9),               ///< Register the F1 key to open the developer console
    Dev_ReloadResources = W_BIT(10),      ///< Register the F4 key to reload all resources
    Dev_ShowStats = W_BIT(11),            ///< Register the F5 key to show stats on screen, such as FPS
    Dev_CaptureProfilingInfo = W_BIT(12), ///< Register the F8 key to write a profiling capture to disk
    Dev_CaptureFrame = W_BIT(13),         ///< Register the F11 key to make a render frame capture (if capture plugin is available)
    Dev_Screenshot = W_BIT(14),           ///< Register the F12 key to save a screenshot to disk
    Dev_OpenInspector = W_BIT(15),        ///< Register the F10 key to open the WInspector application

    Dev_All = 0xFFFFFF00,
    Regular = ~Dev_All,

    Default = All
  };

  struct Bits
  {
    StorageType LoadInputConfig : 1;
    StorageType NonDevBits : 7;

    StorageType Dev_EscapeToClose : 1;
    StorageType Dev_Console : 1;
    StorageType Dev_ReloadResources : 1;
    StorageType Dev_ShowStats : 1;
    StorageType Dev_CaptureProfilingInfo : 1;
    StorageType Dev_CaptureFrame : 1;
    StorageType Dev_Screenshot : 1;
    StorageType Dev_OpenInspector : 1;
  };
};

/// The base class for all typical game applications made with WorldEngine
///
/// While WApplication is an abstraction for the operating system entry point,
/// WGameApplication extends this to implement startup and tear down functionality
/// of a typical game that uses the standard functionality of WorldEngine.
///
/// WGameApplication implements a lot of functionality needed by most games,
/// such as setting up data directories, loading plugins, configuring the input system, etc.
///
/// For every such step a virtual function is called, allowing to override steps in custom applications.
///
/// The default implementation tries to do as much of this in a data-driven way. E.g. plugin and data
/// directory configurations are read from DDL files. These can be configured by hand or using WEditor.
///
/// You are NOT supposed to implement game functionality by deriving from WGameApplication.
/// Instead see WGameState.
///
/// WGameApplication will create exactly one WGameState by looping over all available WGameState types
/// (through reflection) and picking the one that's not marked as a "fallback" gamestate.
/// If none is found, it uses the fallback gamestate instead.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class W_GAMEENGINE_DLL WGameApplication : public WGameApplicationBase
{
public:
  static WCVarBool cvar_AppVSync;
  static WCVarBool cvar_AppShowFPS;
  static WCVarBool cvar_WorldShowObjectOrigins;

public:
  using SUPER = WGameApplicationBase;

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  WGameApplication(const char* szAppName, const char* szProjectPath);
  ~WGameApplication();

  /// Returns the WGameApplication singleton
  static WGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// Returns the active renderer of the current app. Either the default or overridden via -render command line flag.
  static WStringView GetActiveRenderer();

  /// When the graphics device is created, by default the game application will pick a platform specific implementation. This
  /// function allows to override that by setting a custom function that creates a graphics device.
  static void SetOverrideDefaultDeviceCreator(WDelegate<WGALDevice*(const WGALDeviceCreationDescription&)> creator);

  /// Implementation of WGameApplicationBase::FindProjectDirectory to define the 'project' special data directory.
  ///
  /// The default implementation will try to resolve m_sAppProjectPath to an absolute path. m_sAppProjectPath can be absolute itself,
  /// relative to ">sdk/" or relative to WOSFile::GetApplicationDirectory().
  /// m_sAppProjectPath must be set either via the WGameApplication constructor or manually set before project.
  ///
  /// Alternatively, WGameApplication::FindProjectDirectory() must be overwritten.
  virtual WString FindProjectDirectory() const override;

  /// Returns the project path that was given to the constructor (or modified by an overridden implementation).
  WStringView GetAppProjectPath() const { return m_sAppProjectPath; }

  /// Call this to configure which actions the WGameApplication may handle
  ///
  /// Apart from loading the input configuration from disk, this mostly sets up developer options.
  /// The function is typically called by WGameState::ConfigureInputActions().
  /// Once you need full control over the keyboard bindings, you should write your own game state
  /// and override WGameState::ConfigureInputActions() to not register all the developer options.
  void RegisterGameApplicationInputActions(WBitflags<WGameApplicationInputFlags> flags);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  /// Whether the application was started with any of the unattended command line options.
  ///
  /// Unattended mode is what makes an application usable as a smoke test from a script: it quits on its own
  /// ('-runframes', '-timeout'), writes a screenshot ('-screenshot') and a log ('-logfile'), and reports
  /// logged errors through the return code ('-failonerror'). '-fixedtimestep' and '-seed' additionally make
  /// consecutive runs produce the same frames, so that a screenshot can be compared against a reference.
  ///
  /// The options only exist in development builds.
  bool IsUnattended() const { return m_bUnattended; }
#endif

public:
  virtual void Run() override;

protected:
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;
  virtual void StoreScreenshot(WImage&& image, WStringView sContext = {}) override;

  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual WGameUpdateMode GetGameUpdateMode() const override;

  virtual bool Run_ProcessApplicationInput() override;
  virtual void Run_AcquireImage() override;
  virtual void Run_WorldUpdateAndRender() override;
  virtual void Run_PresentImage() override;
  virtual void Run_FinishFrame() override;

  /// Stores what is given to the constructor
  WString m_sAppProjectPath;

protected:
  static WGameApplication* s_pGameApplicationInstance;

  void RenderWorldDebugInfos(const WWorld& world);
  void RenderFps();
  void RenderConsole();
  void OpenInspector();

  void UpdateWorldsAndExtractViews();
  WSharedPtr<WDelegateTask<void>> m_pUpdateTask;

  static WDelegate<WGALDevice*(const WGALDeviceCreationDescription&)> s_DefaultDeviceCreator;

  bool m_bShowConsole = false;
  WUniquePtr<WConsole> m_pConsole;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
protected:
  /// Reads the unattended options and starts writing the log file. Called before anything else may log.
  void Unattended_Setup();

  /// Starts the frame counting, the timeout and the deterministic mode. Called once the game state is active.
  void Unattended_Start();

  /// Applies '-failonerror' to the return code. Called during shutdown, while the log still exists.
  void Unattended_Finish();

  /// Detaches the log writers of '-logfile' and '-failonerror'. Called after all logging is done.
  void Unattended_DetachLog();

  /// Quits with return code 2 if '-timeout' has elapsed.
  ///
  /// Checked outside the frame events, because those don't fire while the window is minimized or while
  /// the application hangs during startup.
  void Unattended_CheckTimeout();

  /// Writes the image to the path given with '-screenshot'. Returns false if no path was given.
  ///
  /// The file is written synchronously and through WOSFile, so that it is guaranteed to exist when the
  /// process exits, and so that the path does not have to be inside a writable data directory.
  bool Unattended_StoreScreenshot(WImage& ref_image);

  void Unattended_OnExecutionEvent(const WGameApplicationExecutionEvent& e);
  void Unattended_OnLogEvent(const WLoggingEventData& e);

  bool m_bUnattended = false;
  bool m_bFailOnError = false;
  bool m_bScreenshotRequested = false;
  bool m_bScreenshotDone = false;
  WInt32 m_iRunFrames = -1;  ///< negative = run until the application is quit normally
  WInt32 m_iRandomSeed = -1; ///< negative = don't touch the random number generators
  WTime m_UnattendedTimeout; ///< zero = disabled
  WTime m_UnattendedStartTime;
  WTime m_FixedTimeStep;     ///< zero = use the real elapsed time
  WString m_sScreenshotPath; ///< empty = don't take one
  WUInt32 m_uiRenderedFrames = 0;
  WAtomicInteger32 m_iLoggedErrors;
  WLogWriter::TextFile m_UnattendedLogFile;
  WEventSubscriptionID m_UnattendedExecutionEventsID = 0;
  WEventSubscriptionID m_UnattendedLogToFileID = 0;
  WEventSubscriptionID m_UnattendedLogErrorCounterID = 0;
#endif
};

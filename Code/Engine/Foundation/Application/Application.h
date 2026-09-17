#pragma once

/// \file

#include <Foundation/Basics.h>

#include <Foundation/Application/Implementation/ApplicationEntryPoint.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <Foundation/Profiling/Profiling.h>

class WApplication;

/// Platform independent run function for main loop based systems (e.g. Win32, ..)
///
/// This is automatically called by W_APPLICATION_ENTRY_POINT().
///
/// WRun simply calls WRun_Startup(), WRun_MainLoop() and WRun_Shutdown().
W_FOUNDATION_DLL void WRun(WApplication* pApplicationInstance);

/// [internal] Called by WRun()
W_FOUNDATION_DLL WResult WRun_Startup(WApplication* pApplicationInstance);
/// [internal] Called by WRun()
W_FOUNDATION_DLL void WRun_MainLoop(WApplication* pApplicationInstance);
/// [internal] Called by WRun()
W_FOUNDATION_DLL void WRun_Shutdown(WApplication* pApplicationInstance);

/// Base class to be used by applications based on WorldEngine.
///
/// The platform abstraction layer will ensure that the correct functions are called independent of the basic main loop structure
/// (traditional or event-based). Derive an application specific class from WApplication and implement at least the abstract Run()
/// function. Additional virtual functions allow to hook into specific events to run application specific code at the correct times.
///
/// Finally pass the name of your derived class to the macro W_APPLICATION_ENTRY_POINT().
/// Those are used to abstract away the platform specific code to run an application.
///
/// A simple example how to get started is as follows:
///
/// \code{.cpp}
///   class WSampleApp : public WApplication
///   {
///   public:
///
///     virtual void AfterCoreSystemsStartup() override
///     {
///       // Setup Filesystem, Logging, etc.
///     }
///
///     virtual void BeforeCoreSystemsShutdown() override
///     {
///       // Close log file, etc.
///     }
///
///     virtual void Run() override
///     {
///       // Run will be called repeatedly, until QuitApplication() has been called (at any time in the frame).
///
///       QuitApplication();
///     }
///   };
///
///   W_APPLICATION_ENTRY_POINT(WSampleApp);
/// \endcode
class W_FOUNDATION_DLL WApplication
{
  W_DISALLOW_COPY_AND_ASSIGN(WApplication);

public:
  /// Constructor.
  WApplication(WStringView sAppName);

  /// Virtual destructor.
  virtual ~WApplication();

  /// Changes the application name
  void SetApplicationName(WStringView sAppName);

  /// Returns the application name
  const WString& GetApplicationName() const { return m_sAppName; }

  /// This function is called before any kind of engine initialization is done.
  ///
  /// Override this function to be able to configure subsystems, before they are initialized.
  /// After this function returns, WStartup::StartupCoreSystems() is automatically called.
  /// If you need to set up custom allocators, this is the place to do this.
  virtual WResult BeforeCoreSystemsStartup();

  /// This function is called after basic engine initialization has been done.
  ///
  /// WApplication will automatically call WStartup::StartupCoreSystems() to initialize the application.
  /// This function can be overridden to do additional application specific initialization.
  /// To startup entire subsystems, you should however use the features provided by WStartup and WSubSystem.
  virtual void AfterCoreSystemsStartup() {}

  /// This function is called after the application main loop has run for the last time, before engine deinitialization.
  ///
  /// After this function call, WApplication executes WStartup::ShutdownHighLevelSystems().
  ///
  /// \note WApplication does NOT call WStartup::StartupHighLevelSystems() as it may be a window-less application.
  /// This is left to WGameApplicationBase to do. However, it does make sure to shut down the high-level systems,
  /// in case they were started.
  virtual void BeforeHighLevelSystemsShutdown() {}

  /// Called after WStartup::ShutdownHighLevelSystems() has been executed.
  virtual void AfterHighLevelSystemsShutdown() {}

  /// This function is called after the application main loop has run for the last time, before engine deinitialization.
  ///
  /// Override this function to do application specific deinitialization that still requires a running engine.
  /// After this function returns WStartup::ShutdownCoreSystems() is called and thus everything, including allocators, is shut down.
  /// To shut down entire subsystems, you should, however, use the features provided by WStartup and WSubSystem.
  virtual void BeforeCoreSystemsShutdown() {}

  /// This function is called after WStartup::ShutdownCoreSystems() has been called.
  ///
  /// It is unlikely that there is any kind of deinitialization left, that can still be run at this point.
  virtual void AfterCoreSystemsShutdown() {}

  /// This function is called when an application is moved to the background.
  ///
  /// On Windows that might simply mean that the main window lost the focus.
  /// On other devices this might mean that the application is not visible at all anymore and
  /// might even get shut down later. Override this function to be able to put the application
  /// into a proper sleep mode.
  virtual void BeforeEnterBackground() {}

  /// This function is called whenever an application is resumed from background mode.
  ///
  /// On Windows that might simply mean that the main window received focus again.
  /// On other devices this might mean that the application was suspended and is now active again.
  /// Override this function to reload the apps state or other resources, etc.
  virtual void BeforeEnterForeground() {}

  /// Main run function which is called periodically. This function must be overridden.
  ///
  /// Call QuitApplication() at any point to prevent Run() from being called again.
  virtual void Run() = 0;

  /// Sets the value that the application will return to the OS.
  /// You can call this function at any point during execution to update the return value of the application.
  /// Default is zero.
  inline void SetReturnCode(WInt32 iReturnCode) { m_iReturnCode = iReturnCode; }

  /// Returns the currently set value that the application will return to the OS.
  inline WInt32 GetReturnCode() const { return m_iReturnCode; }

  /// If the return code is not zero, this function might be called to get a string to print the error code in human readable form.
  virtual const char* TranslateReturnCode() const { return ""; }

  /// Will set the command line arguments that were passed to the app by the OS.
  /// This is automatically called by W_APPLICATION_ENTRY_POINT().
  void SetCommandLineArguments(WUInt32 uiArgumentCount, const char** pArguments);

  /// Returns the one instance of WApplication that is available.
  static WApplication* GetApplicationInstance() { return s_pApplicationInstance; }

  /// Returns the number of command line arguments that were passed to the application.
  ///
  /// Note that the very first command line argument is typically the path to the application itself.
  WUInt32 GetArgumentCount() const { return m_uiArgumentCount; }

  /// Returns one of the command line arguments that was passed to the application.
  const char* GetArgument(WUInt32 uiArgument) const;

  /// Returns the complete array of command line arguments that were passed to the application.
  const char** GetArgumentsArray() const { return m_pArguments; }

  void EnableMemoryLeakReporting(bool bEnable) { m_bReportMemoryLeaks = bEnable; }

  bool IsMemoryLeakReportingEnabled() const { return m_bReportMemoryLeaks; }

  /// Tells the application to shut itself down.
  void QuitApplication() { m_bQuitApplication = true; }

  /// Whether the application should shut down.
  ///
  /// By default just reports whether QuitApplication() was called.
  /// Overridden implementations can take additional things into consideration.
  virtual bool ShouldApplicationQuit() const { return m_bQuitApplication; }

private:
  WInt32 m_iReturnCode = 0;

  WUInt32 m_uiArgumentCount = 0;

  const char** m_pArguments = nullptr;

  bool m_bQuitApplication = false;
  bool m_bReportMemoryLeaks = true;

  WString m_sAppName;

  static WApplication* s_pApplicationInstance;

  friend W_FOUNDATION_DLL_FRIEND void WRun(WApplication* pApplicationInstance);
  friend W_FOUNDATION_DLL_FRIEND WResult WRun_Startup(WApplication* pApplicationInstance);
  friend W_FOUNDATION_DLL_FRIEND void WRun_MainLoop(WApplication* pApplicationInstance);
  friend W_FOUNDATION_DLL_FRIEND void WRun_Shutdown(WApplication* pApplicationInstance);
};

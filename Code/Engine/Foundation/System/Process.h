#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

using WOsProcessHandle = void*;
using WOsProcessID = WUInt32;

#if W_ENABLED(W_SUPPORTS_PROCESSES)
enum class WProcessState
{
  NotStarted,
  Running,
  Finished
};

/// Options that describe how to run an external process
struct W_FOUNDATION_DLL WProcessOptions
{
  /// Path to the binary to launch
  WString m_sProcess;

  /// Custom working directory for the launched process. If empty, inherits the CWD from the parent process.
  WString m_sWorkingDirectory;

  /// Arguments to pass to the process. Strings that contain spaces will be wrapped in quotation marks automatically
  WHybridArray<WString, 8> m_Arguments;

  /// If set to true, command line tools will not show their console window, but execute in the background
  bool m_bHideConsoleWindow = true;

  /// If set, stdout will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  WDelegate<void(WStringView)> m_onStdOut;

  /// If set, stderr will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  WDelegate<void(WStringView)> m_onStdError;

  /// Appends a formatted argument to m_Arguments
  ///
  /// This can be useful if a complex command needs to be added as a single argument.
  /// Ie. since arguments with spaces will be wrapped in quotes, it can make a difference
  /// whether a complex parameter is added as one or multiple arguments.
  void AddArgument(const WFormatString& arg);

  /// Overload of AddArgument(WFormatString) for convenience.
  template <typename... ARGS>
  void AddArgument(WStringView sFormat, ARGS&&... args)
  {
    AddArgument(WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Takes a full command line and appends it as individual arguments by splitting it along white-space and quotation marks.
  ///
  /// Brief, use this, if arguments are already pre-built as a full command line.
  void AddCommandLine(WStringView sCmdLine);

  /// Builds the command line from the process arguments and appends it to \a out_sCmdLine.
  void BuildCommandLineString(WStringBuilder& out_sCmdLine) const;
};

/// Flags for WProcess::Launch()
struct WProcessLaunchFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,
    Detached = W_BIT(0),  ///< The process will be detached right after launch, as if WProcess::Detach() was called.
    Suspended = W_BIT(1), ///< The process will be launched in a suspended state. Call WProcess::ResumeSuspended() to unpause it.
    Default = None
  };

  struct Bits
  {
    StorageType Detached : 1;
    StorageType Suspended : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WProcessLaunchFlags);

/// Provides functionality to launch other processes
class W_FOUNDATION_DLL WProcess
{
  W_DISALLOW_COPY_AND_ASSIGN(WProcess);

public:
  WProcess();
  WProcess(WProcess&& rhs);

  /// Upon destruction the running process will be terminated.
  ///
  /// Use Detach() to prevent the termination of the launched process.
  ///
  /// \sa Terminate()
  /// \sa Detach()
  ~WProcess();

  /// Launches the specified process and waits for it to finish.
  static WResult Execute(const WProcessOptions& opt, WInt32* out_pExitCode = nullptr);

  /// Launches the specified process asynchronously.
  ///
  /// When the function returns, the process is typically starting or running.
  /// Call WaitToFinish() to wait for the process to shutdown or Terminate() to kill it.
  ///
  /// \sa WProcessLaunchFlags
  WResult Launch(const WProcessOptions& opt, WBitflags<WProcessLaunchFlags> launchFlags = WProcessLaunchFlags::None);

  /// Resumes a process that was launched in a suspended state. Returns W_FAILURE if the process has not been launched or already
  /// resumed.
  WResult ResumeSuspended();

  /// Waits the given amount of time for the previously launched process to finish.
  ///
  /// Pass in WTime::MakeZero() to wait indefinitely.
  /// Returns W_FAILURE, if the process did not finish within the given time.
  ///
  /// \note Asserts that the WProcess instance was used to successfully launch a process before.
  WResult WaitToFinish(WTime timeout = WTime::MakeZero());

  /// Kills the detached process, if possible.
  WResult Terminate();

  /// Returns the exit code of the process. The exit code will be -0xFFFF as long as the process has not finished.
  WInt32 GetExitCode() const;

  /// Returns the running state of the process
  ///
  /// If the state is 'finished' the exit code (as returned by GetExitCode() ) will be updated.
  WProcessState GetState() const;

  /// Detaches the running process from the WProcess instance.
  ///
  /// This means the WProcess instance loses control over terminating the process or communicating with it.
  /// It also means that the process will keep running and not get terminated when the WProcess instance is destroyed.
  void Detach();

  /// Returns the OS specific handle to the process
  WOsProcessHandle GetProcessHandle() const;

  /// Returns the OS-specific process ID (PID)
  WOsProcessID GetProcessID() const;

  /// Returns OS-specific process ID (PID) for the calling process
  static WOsProcessID GetCurrentProcessID();

private:
  void BuildFullCommandLineString(const WProcessOptions& opt, WStringView sProcess, WStringBuilder& cmd) const;

  WUniquePtr<struct WProcessImpl> m_pImpl;

  // the default value is used by GetExitCode() to determine whether it has to be reevaluated
  mutable WInt32 m_iExitCode = -0xFFFF;

  WString m_sProcess;
  WDelegate<void(WStringView)> m_OnStdOut;
  WDelegate<void(WStringView)> m_OnStdError;
  mutable WTime m_ProcessExited = WTime::MakeZero();
};
#endif

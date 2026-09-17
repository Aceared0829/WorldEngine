#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

W_WARNING_PUSH()
W_WARNING_DISABLE_MSVC(4355)

#ifndef W_THREAD_CLASS_ENTRY_POINT
#  error "Definition for WThreadClassEntryPoint is missing on this platform!"
#endif

W_THREAD_CLASS_ENTRY_POINT;

/// Event data for thread lifecycle notifications
struct WThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the WThread instance (not the WThread itself).
    ThreadDestroyed,   ///< Called on the thread that destroys the WThread instance (not the WThread itself).
    StartingExecution, ///< Called on the WThread before the Run() method is executed.
    FinishedExecution, ///< Called on the WThread after the Run() method was executed.
    ClearThreadLocals, ///< Potentially called on the WThread (currently only for task system threads) at a time when plugins should clean up thread-local storage.
  };

  Type m_Type;
  WThread* m_pThread = nullptr;
};

/// This class is the base class for platform independent long running threads
///
/// Used by deriving from this class and overriding the Run() method.
class W_FOUNDATION_DLL WThread : public WOSThread
{
public:
  /// Returns the current WThread if the current platform thread is an WThread. Returns nullptr otherwise.
  static const WThread* GetCurrentThread();

  /// Thread execution state
  enum WThreadStatus
  {
    Created = 0, ///< Thread created but not yet started
    Running,     ///< Thread is currently executing
    Finished     ///< Thread execution has completed
  };

  /// Creates a new thread with specified name and stack size
  ///
  /// The thread is created in Created state and must be started separately.
  /// Default stack size of 128KB is suitable for most purposes.
  WThread(WStringView sName = "WThread", WUInt32 uiStackSize = 128 * 1024);

  /// Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~WThread();

  /// Returns the thread status
  inline WThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// Returns the thread name
  inline const char* GetThreadName() const { return m_sName.GetData(); }

  /// Global events for thread lifecycle monitoring
  ///
  /// These events inform about threads starting and finishing. Events are raised on the executing thread,
  /// allowing thread-specific initialization and cleanup code to be executed during callbacks.
  /// Useful for setting up thread-local storage or registering threads with profiling systems.
  static WEvent<const WThreadEvent&, WMutex> s_ThreadEvents;

private:
  /// Pure virtual function that contains the thread's main execution logic
  ///
  /// Override this method to implement the work that the thread should perform.
  /// The return value is passed as the thread exit code and can be retrieved after the thread finishes.
  virtual WUInt32 Run() = 0;


  volatile WThreadStatus m_ThreadStatus = Created;

  WString m_sName;

  friend WUInt32 RunThread(WThread* pThread);
};

W_WARNING_POP()

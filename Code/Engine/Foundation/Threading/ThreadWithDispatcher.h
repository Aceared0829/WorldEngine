#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Thread.h>

/// Thread base class enabling cross-thread function call dispatching
///
/// Extends WThread to provide a message passing mechanism where other threads can schedule
/// function calls to execute within this thread's context. Useful for thread-safe operations
/// that must run on specific threads (e.g., UI updates, OpenGL calls).
///
/// Derived classes must call DispatchQueue() regularly in their Run() method to process
/// queued function calls. The double-buffering design ensures minimal lock contention.
class W_FOUNDATION_DLL WThreadWithDispatcher : public WThread
{
public:
  using DispatchFunction = WDelegate<void(), 128>;

  /// Initializes the runnable class
  WThreadWithDispatcher(const char* szName = "WThreadWithDispatcher", WUInt32 uiStackSize = 128 * 1024);

  /// Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~WThreadWithDispatcher();

  /// Use this to enqueue a function call to the given delegate at some later point running in the given thread context.
  void Dispatch(DispatchFunction&& delegate);

protected:
  /// Needs to be called by derived thread implementations to dispatch the function calls.
  void DispatchQueue();

private:
  /// The run function can be used to implement a long running task in a thread in a platform independent way
  virtual WUInt32 Run() = 0;

  WDynamicArray<DispatchFunction> m_ActiveQueue;
  WDynamicArray<DispatchFunction> m_CurrentlyBeingDispatchedQueue;

  WMutex m_QueueMutex;
};

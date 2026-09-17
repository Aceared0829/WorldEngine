#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>
#include <Foundation/Threading/Mutex.h>

/// Condition variables are used to put threads to sleep and wake them up upon certain events
///
/// The WConditionVariable works in conjunction with a mutex. When waiting for a signal,
/// the OS typically puts the waiting thread to sleep.
/// Using SignalOne() or SignalAll() other threads can wake up one or all threads that are
/// currently waiting on the condition variable.
///
/// When a thread is woken up, it automatically holds the lock on the condition variable's mutex,
/// which can be used to safely access or modify certain state.
///
/// WConditionVariable is a low-level threading construct. Higher level functionality such as
/// WThreadSignal may be more suitable for most use cases.
///
/// \sa WThreadSignal, WMutex
class W_FOUNDATION_DLL WConditionVariable
{
  W_DISALLOW_COPY_AND_ASSIGN(WConditionVariable);

public:
  enum class WaitResult
  {
    Signaled, ///< A signal was received before the timeout
    Timeout   ///< The timeout period elapsed without receiving a signal
  };

  WConditionVariable();
  ~WConditionVariable();

  /// Locks the internal mutex. Recursive locking is allowed.
  void Lock();

  /// Tries to lock the internal mutex. Recursive locking is allowed.
  WResult TryLock();

  /// Unlocks the internal mutex. Must be called as often as it was locked.
  void Unlock();

  /// Wakes up one waiting thread
  ///
  /// If no threads are waiting, this has no effect. Due to OS scheduling behavior,
  /// spurious wakeups may occasionally wake more than one thread.
  void SignalOne();

  /// Wakes up all the threads that are currently waiting for the variable.
  ///
  /// If no thread is currently waiting, this has no effect.
  void SignalAll();

  /// Puts the calling thread to sleep and waits for the variable to get signaled.
  ///
  /// Asserts that the WConditionVariable is locked when the function is called.
  /// The mutex will be unlocked and the thread is put to sleep.
  /// When the signal arrives, the thread is woken up and the mutex is locked again.
  void UnlockWaitForSignalAndLock() const;

  /// Same as UnlockWaitForSignalAndLock() but with an additional timeout condition.
  ///
  /// If the timeout is reached before the signal arrived, the function returns with
  /// WaitResult::Timeout.
  ///
  /// \note If the timeout is reached, the mutex will still get locked!
  WaitResult UnlockWaitForSignalAndLock(WTime timeout) const;

private:
  mutable WInt32 m_iLockCount = 0;
  mutable WMutex m_Mutex;
  mutable WConditionVariableData m_Data;
};

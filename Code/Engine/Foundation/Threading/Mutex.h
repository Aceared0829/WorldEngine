#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// Provides a simple mechanism for mutual exclusion to prevent multiple threads from accessing a shared resource simultaneously.
///
/// This can be used to protect code that is not thread-safe against race conditions.
/// To ensure that mutexes are always properly released, use the WLock class or W_LOCK macro.
///
/// \sa WSemaphore, WConditionVariable
class W_FOUNDATION_DLL WMutex
{
  W_DISALLOW_COPY_AND_ASSIGN(WMutex);

public:
  WMutex();
  ~WMutex();

  /// Acquires an exclusive lock for this mutex object
  void Lock();

  /// Attempts to acquire an exclusive lock for this mutex object. Returns true on success.
  ///
  /// If the mutex is already acquired by another thread, the function returns immediately and returns false.
  WResult TryLock();

  /// Releases a lock that has been previously acquired
  void Unlock();

  /// Returns true, if the mutex is currently acquired. Can be used to assert that a lock was entered.
  ///
  /// Obviously, this check is not thread-safe and should not be used to check whether a mutex could be locked without blocking.
  /// Use TryLock for that instead.
  W_ALWAYS_INLINE bool IsLocked() const { return m_iLockCount > 0; }

  WMutexHandle& GetMutexHandle() { return m_hHandle; }

private:
  WMutexHandle m_hHandle;
  WInt32 m_iLockCount = 0;
};

/// A dummy mutex that does no locking.
///
/// Used when a mutex object needs to be passed to some code (such as allocators), but thread-synchronization
/// is actually not necessary.
class W_FOUNDATION_DLL WNoMutex
{
public:
  /// Implements the 'Acquire' interface function, but does nothing.
  W_ALWAYS_INLINE void Lock() {}

  /// Implements the 'TryLock' interface function, but does nothing.
  W_ALWAYS_INLINE WResult TryLock()
  {
    return W_SUCCESS;
  }

  /// Implements the 'Release' interface function, but does nothing.
  W_ALWAYS_INLINE void Unlock() {}

  W_ALWAYS_INLINE bool IsLocked() const
  {
    return false;
  }
};

#include <Mutex_Platform.h>

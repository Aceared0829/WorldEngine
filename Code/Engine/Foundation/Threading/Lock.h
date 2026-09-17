#pragma once

#include <Foundation/Basics.h>

/// Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
/// Works with any object that implements Lock() and Unlock() methods (WMutex, etc.).
/// Use the W_LOCK macro for convenient scoped locking.
template <typename T>
class WLock
{
public:
  /// Acquires the lock immediately upon construction
  W_ALWAYS_INLINE explicit WLock(T& ref_lock)
    : m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  /// Automatically releases the lock when the object is destroyed
  W_ALWAYS_INLINE ~WLock() { m_Lock.Unlock(); }

private:
  WLock();
  WLock(const WLock<T>& rhs);
  void operator=(const WLock<T>& rhs);

  T& m_Lock;
};

/// Convenient macro for creating a scoped lock with automatic type deduction
///
/// Creates an WLock instance with a unique name based on the source line number.
/// The lock is held for the duration of the current scope. Equivalent to:
/// WLock<decltype(lock)> variable_name(lock);
///
/// Example: W_LOCK(myMutex); // Locks myMutex until end of scope
#define W_LOCK(lock) WLock<decltype(lock)> W_PP_CONCAT(l_, W_SOURCE_LINE)(lock)

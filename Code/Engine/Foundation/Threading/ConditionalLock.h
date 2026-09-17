#pragma once

#include <Foundation/Basics.h>

/// RAII lock guard that conditionally acquires and releases locks based on runtime conditions
///
/// Provides the same automatic lock management as WLock but only performs the actual locking
/// when a boolean condition is met. Useful in scenarios where locking is only required under
/// certain circumstances, avoiding unnecessary synchronization overhead when protection is not needed.
///
/// The condition is evaluated once at construction time. If false, no locking occurs throughout
/// the object's lifetime, making this essentially a no-op with zero runtime cost.
template <typename T>
class WConditionalLock
{
public:
  W_ALWAYS_INLINE explicit WConditionalLock(T& lock, bool bCondition)
    : m_lock(lock)
    , m_bCondition(bCondition)
  {
    if (m_bCondition)
    {
      m_lock.Lock();
    }
  }

  W_ALWAYS_INLINE ~WConditionalLock()
  {
    if (m_bCondition)
    {
      m_lock.Unlock();
    }
  }

private:
  WConditionalLock();
  WConditionalLock(const WConditionalLock<T>& rhs);
  void operator=(const WConditionalLock<T>& rhs);

  T& m_lock;
  bool m_bCondition;
};

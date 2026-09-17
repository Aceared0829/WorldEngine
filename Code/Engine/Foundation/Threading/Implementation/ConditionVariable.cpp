#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

void WConditionVariable::Lock()
{
  m_Mutex.Lock();
  ++m_iLockCount;
}

WResult WConditionVariable::TryLock()
{
  if (m_Mutex.TryLock().Succeeded())
  {
    ++m_iLockCount;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WConditionVariable::Unlock()
{
  W_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}

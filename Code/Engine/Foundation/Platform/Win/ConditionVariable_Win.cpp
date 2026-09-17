#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Threading/ConditionVariable.h>
#  include <Foundation/Time/Time.h>

static_assert(sizeof(CONDITION_VARIABLE) == sizeof(WConditionVariableHandle), "not equal!");

WConditionVariable::WConditionVariable()
{
  InitializeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

WConditionVariable::~WConditionVariable()
{
  W_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");
}

void WConditionVariable::SignalOne()
{
  WakeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void WConditionVariable::SignalAll()
{
  WakeAllConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void WConditionVariable::UnlockWaitForSignalAndLock() const
{
  W_ASSERT_DEV(m_iLockCount > 0, "WConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  SleepConditionVariableCS(
    reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable), (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), INFINITE);
}

WConditionVariable::WaitResult WConditionVariable::UnlockWaitForSignalAndLock(WTime timeout) const
{
  W_ASSERT_DEV(m_iLockCount > 0, "WConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;
  DWORD result = SleepConditionVariableCS(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable),
    (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), static_cast<DWORD>(timeout.GetMilliseconds()));

  if (result == WAIT_TIMEOUT)
  {
    // inside the lock at this point
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}

#endif

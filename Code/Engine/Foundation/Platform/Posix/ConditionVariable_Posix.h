#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Time/Time.h>

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

WConditionVariable::WConditionVariable()
{
  pthread_cond_init(&m_Data.m_ConditionVariable, nullptr);
}

WConditionVariable::~WConditionVariable()
{
  W_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");

  pthread_cond_destroy(&m_Data.m_ConditionVariable);
}

void WConditionVariable::SignalOne()
{
  pthread_cond_signal(&m_Data.m_ConditionVariable);
}

void WConditionVariable::SignalAll()
{
  pthread_cond_broadcast(&m_Data.m_ConditionVariable);
}

void WConditionVariable::UnlockWaitForSignalAndLock() const
{
  W_ASSERT_DEV(m_iLockCount > 0, "WConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  pthread_cond_wait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle());
}

WConditionVariable::WaitResult WConditionVariable::UnlockWaitForSignalAndLock(WTime timeout) const
{
  W_ASSERT_DEV(m_iLockCount > 0, "WConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;

  timeval now;
  gettimeofday(&now, nullptr);

  // pthread_cond_timedwait needs an absolute time value, so compute it from the current time.
  struct timespec timeToWait;

  const WInt64 iNanoSecondsPerSecond = 1000000000LL;
  const WInt64 iMicroSecondsPerNanoSecond = 1000LL;

  WInt64 endTime = now.tv_sec * iNanoSecondsPerSecond + now.tv_usec * iMicroSecondsPerNanoSecond + static_cast<WInt64>(timeout.GetNanoseconds());

  timeToWait.tv_sec = endTime / iNanoSecondsPerSecond;
  timeToWait.tv_nsec = endTime % iNanoSecondsPerSecond;

  if (pthread_cond_timedwait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle(), &timeToWait) == ETIMEDOUT)
  {
    // inside the lock
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}

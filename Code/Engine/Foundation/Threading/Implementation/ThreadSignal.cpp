#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Time/Time.h>

WThreadSignal::WThreadSignal(Mode mode /*= Mode::AutoReset*/)
{
  m_Mode = mode;
}

WThreadSignal::~WThreadSignal() = default;

void WThreadSignal::WaitForSignal() const
{
  W_LOCK(m_ConditionVariable);

  while (!m_bSignalState)
  {
    m_ConditionVariable.UnlockWaitForSignalAndLock();
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }
}

WThreadSignal::WaitResult WThreadSignal::WaitForSignal(WTime timeout) const
{
  W_LOCK(m_ConditionVariable);

  const WTime tStart = WTime::Now();
  WTime tElapsed = WTime::MakeZero();

  while (!m_bSignalState)
  {
    if (m_ConditionVariable.UnlockWaitForSignalAndLock(timeout - tElapsed) == WConditionVariable::WaitResult::Timeout)
    {
      return WaitResult::Timeout;
    }

    tElapsed = WTime::Now() - tStart;
    if (tElapsed >= timeout)
    {
      return WaitResult::Timeout;
    }
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }

  return WaitResult::Signaled;
}

void WThreadSignal::RaiseSignal()
{
  {
    W_LOCK(m_ConditionVariable);
    m_bSignalState = true;
  }

  if (m_Mode == Mode::AutoReset)
  {
    // with auto-reset there is no need to wake up more than one
    m_ConditionVariable.SignalOne();
  }
  else
  {
    m_ConditionVariable.SignalAll();
  }
}

void WThreadSignal::ClearSignal()
{
  W_LOCK(m_ConditionVariable);
  m_bSignalState = false;
}

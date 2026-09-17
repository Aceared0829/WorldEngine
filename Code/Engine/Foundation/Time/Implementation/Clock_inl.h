#pragma once

#include <Foundation/Time/Clock.h>

inline void WClock::SetClockName(WStringView sName)
{
  m_sName = sName;
}

inline WStringView WClock::GetClockName() const
{
  return m_sName;
}

inline void WClock::SetTimeStepSmoothing(WTimeStepSmoothing* pSmoother)
{
  m_pTimeStepSmoother = pSmoother;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline WTimeStepSmoothing* WClock::GetTimeStepSmoothing() const
{
  return m_pTimeStepSmoother;
}

inline void WClock::SetPaused(bool bPaused)
{
  m_bPaused = bPaused;

  // when we enter a pause, inform the time step smoother to throw away his statistics
  if (bPaused && m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline bool WClock::GetPaused() const
{
  return m_bPaused;
}

inline WTime WClock::GetFixedTimeStep() const
{
  return m_FixedTimeStep;
}

inline WTime WClock::GetAccumulatedTime() const
{
  return m_AccumulatedTime;
}

inline WTime WClock::GetTimeDiff() const
{
  return m_LastTimeDiff;
}

inline double WClock::GetSpeed() const
{
  return m_fSpeed;
}

inline void WClock::SetMinimumTimeStep(WTime min)
{
  W_ASSERT_DEV(min >= WTime::MakeFromSeconds(0.0), "Time flows in one direction only.");

  m_MinTimeStep = min;
}

inline void WClock::SetMaximumTimeStep(WTime max)
{
  W_ASSERT_DEV(max >= WTime::MakeFromSeconds(0.0), "Time flows in one direction only.");

  m_MaxTimeStep = max;
}

inline WTime WClock::GetMinimumTimeStep() const
{
  return m_MinTimeStep;
}

inline WTime WClock::GetMaximumTimeStep() const
{
  return m_MaxTimeStep;
}

inline void WClock::SetFixedTimeStep(WTime diff)
{
  W_ASSERT_DEV(m_FixedTimeStep.GetSeconds() >= 0.0, "Fixed Time Stepping cannot reverse time!");

  m_FixedTimeStep = diff;
}

inline void WClock::SetSpeed(double fFactor)
{
  W_ASSERT_DEV(fFactor >= 0.0, "Time cannot run backwards.");

  m_fSpeed = fFactor;
}

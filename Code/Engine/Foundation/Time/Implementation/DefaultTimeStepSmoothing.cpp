#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

WDefaultTimeStepSmoothing::WDefaultTimeStepSmoothing()
{
  m_fLerpFactor = 0.2f;
}

void WDefaultTimeStepSmoothing::Reset(const WClock* pClock)
{
  W_IGNORE_UNUSED(pClock);

  m_LastTimeSteps.Clear();
}

WTime WDefaultTimeStepSmoothing::GetSmoothedTimeStep(WTime rawTimeStep, const WClock* pClock)
{
  rawTimeStep = WMath::Clamp(rawTimeStep * pClock->GetSpeed(), pClock->GetMinimumTimeStep(), pClock->GetMaximumTimeStep());

  if (m_LastTimeSteps.GetCount() < 10)
  {
    m_LastTimeSteps.PushBack(rawTimeStep);
    m_LastTimeStepTaken = rawTimeStep;
    return m_LastTimeStepTaken;
  }

  if (!m_LastTimeSteps.CanAppend(1))
    m_LastTimeSteps.PopFront(1);

  m_LastTimeSteps.PushBack(rawTimeStep);

  WStaticArray<WTime, 11> Sorted;
  Sorted.SetCountUninitialized(m_LastTimeSteps.GetCount());

  for (WUInt32 i = 0; i < m_LastTimeSteps.GetCount(); ++i)
    Sorted[i] = m_LastTimeSteps[i];

  Sorted.Sort();

  WUInt32 uiFirstSample = 2;
  WUInt32 uiLastSample = 8;

  WTime tAvg;

  for (WUInt32 i = uiFirstSample; i <= uiLastSample; ++i)
  {
    tAvg = tAvg + Sorted[i];
  }

  tAvg = tAvg / (double)((uiLastSample - uiFirstSample) + 1.0);


  m_LastTimeStepTaken = WMath::Lerp(m_LastTimeStepTaken, tAvg, m_fLerpFactor);

  return m_LastTimeStepTaken;
}

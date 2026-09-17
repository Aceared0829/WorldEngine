#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Stopwatch.h>

WStopwatch::WStopwatch()
{
  m_LastCheckpoint = WTime::Now();

  StopAndReset();
  Resume();
}

void WStopwatch::StopAndReset()
{
  m_TotalDuration = WTime::MakeZero();
  m_bRunning = false;
}

void WStopwatch::Resume()
{
  if (m_bRunning)
    return;

  m_bRunning = true;
  m_LastUpdate = WTime::Now();
}

void WStopwatch::Pause()
{
  if (!m_bRunning)
    return;

  m_bRunning = false;

  m_TotalDuration += WTime::Now() - m_LastUpdate;
}

WTime WStopwatch::GetRunningTotal() const
{
  if (m_bRunning)
  {
    const WTime tNow = WTime::Now();

    m_TotalDuration += tNow - m_LastUpdate;
    m_LastUpdate = tNow;
  }

  return m_TotalDuration;
}

WTime WStopwatch::Checkpoint()
{
  const WTime tNow = WTime::Now();

  const WTime tDiff = tNow - m_LastCheckpoint;
  m_LastCheckpoint = tNow;

  return tDiff;
}

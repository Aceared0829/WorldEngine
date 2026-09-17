#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>

WClock::Event WClock::s_TimeEvents;
WClock* WClock::s_pGlobalClock = nullptr;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Clock)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASESYSTEMS_STARTUP
  {
    WClock::s_pGlobalClock = new WClock("Global");
  }

W_END_SUBSYSTEM_DECLARATION;

W_BEGIN_STATIC_REFLECTED_TYPE(WClock, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    W_ACCESSOR_PROPERTY("Speed", GetSpeed, SetSpeed),
  }
  W_END_PROPERTIES;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalClock),
    W_SCRIPT_FUNCTION_PROPERTY(GetAccumulatedTime),
    W_SCRIPT_FUNCTION_PROPERTY(GetTimeDiff)
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WClock::WClock(WStringView sName)
{
  SetClockName(sName);

  Reset(true);
}

void WClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep = WTime::MakeFromSeconds(0.001); // 1000 FPS
    m_MaxTimeStep = WTime::MakeFromSeconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep = WTime::MakeFromSeconds(0.0);
  }

  m_AccumulatedTime = WTime::MakeFromSeconds(0.0);
  m_fSpeed = 1.0;
  m_bPaused = false;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = WTime::Now() - m_MinTimeStep;
  m_LastTimeDiff = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void WClock::Update()
{
  const WTime tNow = WTime::Now();
  const WTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = WTime::MakeFromSeconds(0.0);
  }
  else if (m_FixedTimeStep > WTime::MakeFromSeconds(0.0))
  {
    // scale the time step by the speed factor
    m_LastTimeDiff = m_FixedTimeStep * m_fSpeed;
  }
  else
  {
    // in variable time step mode, apply the time step smoother, if available
    if (m_pTimeStepSmoother)
      m_LastTimeDiff = m_pTimeStepSmoother->GetSmoothedTimeStep(tDiff, this);
    else
    {
      // scale the time step by the speed factor
      // and make sure the time step does not leave the predetermined bounds
      m_LastTimeDiff = WMath::Clamp(tDiff * m_fSpeed, m_MinTimeStep, m_MaxTimeStep);
    }
  }

  m_AccumulatedTime += m_LastTimeDiff;

  EventData ed;
  ed.m_sClockName = m_sName;
  ed.m_RawTimeStep = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void WClock::SetAccumulatedTime(WTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = WTime::Now() - WTime::MakeFromSeconds(0.01);
  m_LastTimeDiff = WTime::MakeFromSeconds(0.01);
}

void WClock::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;

  inout_stream << uiVersion;
  inout_stream << m_AccumulatedTime;
  inout_stream << m_LastTimeDiff;
  inout_stream << m_FixedTimeStep;
  inout_stream << m_MinTimeStep;
  inout_stream << m_MaxTimeStep;
  inout_stream << m_fSpeed;
  inout_stream << m_bPaused;
}

void WClock::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion == 1, "Wrong version for WClock: {0}", uiVersion);

  inout_stream >> m_AccumulatedTime;
  inout_stream >> m_LastTimeDiff;
  inout_stream >> m_FixedTimeStep;
  inout_stream >> m_MinTimeStep;
  inout_stream >> m_MaxTimeStep;
  inout_stream >> m_fSpeed;
  inout_stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = WTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}



W_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);

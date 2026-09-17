#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WUpdateRate, 1)
  W_ENUM_CONSTANTS(WUpdateRate::EveryFrame)
  W_ENUM_CONSTANTS(WUpdateRate::Max30fps, WUpdateRate::Max20fps, WUpdateRate::Max10fps)
  W_ENUM_CONSTANTS(WUpdateRate::Max5fps, WUpdateRate::Max2fps, WUpdateRate::Max1fps)
  W_ENUM_CONSTANTS(WUpdateRate::Never)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

static WTime s_Intervals[] = {
  WTime::MakeZero(),                  // EveryFrame
  WTime::MakeFromSeconds(1.0 / 30.0), // Max30fps
  WTime::MakeFromSeconds(1.0 / 20.0), // Max20fps
  WTime::MakeFromSeconds(1.0 / 10.0), // Max10fps
  WTime::MakeFromSeconds(1.0 / 5.0),  // Max5fps
  WTime::MakeFromSeconds(1.0 / 2.0),  // Max2fps
  WTime::MakeFromSeconds(1.0 / 1.0),  // Max1fps
};

static_assert(W_ARRAY_SIZE(s_Intervals) == WUpdateRate::Max1fps + 1);

WTime WUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

WIntervalSchedulerBase::WIntervalSchedulerBase(WTime minInterval, WTime maxInterval)
  : m_MinInterval(minInterval)
  , m_MaxInterval(maxInterval)
{
  W_ASSERT_DEV(m_MinInterval.IsPositive(), "Min interval must be greater than zero");
  W_ASSERT_DEV(m_MaxInterval > m_MinInterval, "Max interval must be greater than min interval");

  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (WUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

WIntervalSchedulerBase::~WIntervalSchedulerBase() = default;


W_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);

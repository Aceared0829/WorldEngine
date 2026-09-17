#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct W_CORE_DLL WUpdateRate
{
  using StorageType = WUInt8;

  enum Enum
  {
    EveryFrame,
    Max30fps,
    Max20fps,
    Max10fps,
    Max5fps,
    Max2fps,
    Max1fps,
    Never,

    Default = Max30fps
  };

  static WTime GetInterval(Enum updateRate);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// Helper class to schedule work in intervals typically larger than the duration of one frame
///
/// Tries to maintain an even workload per frame and also keep the given interval for a work as best as possible.
/// A typical use case would be e.g. component update functions that don't need to be called every frame.
class W_CORE_DLL WIntervalSchedulerBase
{
protected:
  WIntervalSchedulerBase(WTime minInterval, WTime maxInterval);
  ~WIntervalSchedulerBase();

  WUInt32 GetHistogramIndex(WTime value);
  WTime GetHistogramSlotValue(WUInt32 uiIndex);

  static float GetRandomZeroToOne(int pos, WUInt32& seed);
  static WTime GetRandomTimeJitter(int pos, WUInt32& seed);

  WTime m_MinInterval;
  WTime m_MaxInterval;
  double m_fInvIntervalRange;

  WTime m_CurrentTime;

  WUInt32 m_uiSeed = 0;

  static constexpr WUInt32 HistogramSize = 32;
  WUInt32 m_Histogram[HistogramSize] = {};
  WTime m_HistogramSlotValues[HistogramSize] = {};
};

//////////////////////////////////////////////////////////////////////////

/// \see WIntervalSchedulerBase
template <typename T>
class WIntervalScheduler : public WIntervalSchedulerBase
{
  using SUPER = WIntervalSchedulerBase;

public:
  W_ALWAYS_INLINE WIntervalScheduler(WTime minInterval = WTime::MakeFromMilliseconds(1), WTime maxInterval = WTime::MakeFromSeconds(1))
    : SUPER(minInterval, maxInterval)
  {
  }

  void AddOrUpdateWork(const T& work, WTime interval);
  void RemoveWork(const T& work);

  WTime GetInterval(const T& work) const;

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = WDelegate<void(const T&, WTime)>;

  /// Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(WTime deltaTime, RunWorkCallback runWorkCallback);

  void Clear();

private:
  struct Data
  {
    T m_Work;
    WTime m_Interval;
    WTime m_DueTime;
    WTime m_LastScheduledTime;

    bool IsValid() const;
    void MarkAsInvalid();
  };

  using DataMap = WMap<WTime, Data>;
  DataMap m_Data;
  WHashTable<T, typename DataMap::Iterator> m_WorkIdToData;

  typename DataMap::Iterator InsertData(Data& data);
  WDynamicArray<typename DataMap::Iterator> m_ScheduledWork;
};

#include <Core/Utils/Implementation/IntervalScheduler_inl.h>

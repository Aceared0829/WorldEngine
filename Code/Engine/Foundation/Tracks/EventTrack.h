#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Time.h>

/// An event track is a time line that contains named events.
///
/// The time line can be sampled to query all events that occurred during a time period.
/// There is no way to sample an event track at a fixed point in time, because events occur at specific time points and thus
/// only range queries make sense.
class W_FOUNDATION_DLL WEventTrack
{
public:
  WEventTrack();
  ~WEventTrack();

  /// Removes all control points.
  void Clear();

  /// Checks whether there are any control points in the track.
  bool IsEmpty() const;

  /// Adds a named event into the track at the given time.
  void AddControlPoint(WTime time, WStringView sEvent);

  /// Samples the event track from range [start; end) and adds all events that occured in that time period to the array.
  ///
  /// Note that the range is inclusive for the start time, and exclusive for the end time.
  ///
  /// If rangeStart is larger than rangeEnd, the events are returned in reverse order (backwards traversal).
  void Sample(WTime rangeStart, WTime rangeEnd, WDynamicArray<WHashedString>& out_events) const;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);

private:
  struct ControlPoint
  {
    W_ALWAYS_INLINE bool operator<(const ControlPoint& rhs) const { return m_Time < rhs.m_Time; }

    WTime m_Time;
    WUInt32 m_uiEvent;
  };

  WUInt32 FindControlPointAfter(WTime x) const;
  WInt32 FindControlPointBefore(WTime x) const;

  mutable bool m_bSort = false;
  mutable WDynamicArray<ControlPoint> m_ControlPoints;
  WHybridArray<WHashedString, 4> m_Events;
};

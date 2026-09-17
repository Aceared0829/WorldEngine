#include <Foundation/FoundationPCH.h>

#include <Foundation/Tracks/EventTrack.h>

WEventTrack::WEventTrack() = default;

WEventTrack::~WEventTrack() = default;

void WEventTrack::Clear()
{
  m_Events.Clear();
  m_ControlPoints.Clear();
}

bool WEventTrack::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

void WEventTrack::AddControlPoint(WTime time, WStringView sEvent)
{
  m_bSort = true;

  const WUInt32 uiNumEvents = m_Events.GetCount();

  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_Time = time;

  // search for existing event
  {
    WTempHashedString tmp(sEvent);

    for (WUInt32 i = 0; i < uiNumEvents; ++i)
    {
      if (m_Events[i] == tmp)
      {
        cp.m_uiEvent = i;
        return;
      }
    }
  }

  // not found -> add event name
  {
    cp.m_uiEvent = uiNumEvents;

    WHashedString hs;
    hs.Assign(sEvent);

    m_Events.PushBack(hs);
  }
}

WUInt32 WEventTrack::FindControlPointAfter(WTime x) const
{
  // searches for a control point after OR AT x

  W_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  WUInt32 uiLowIdx = 0;
  WUInt32 uiHighIdx = m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const WUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (WUInt32 idx = uiLowIdx; idx <= uiHighIdx; ++idx)
  {
    if (m_ControlPoints[idx].m_Time >= x)
    {
      return idx;
    }
  }

  W_ASSERT_DEBUG(uiHighIdx + 1 == m_ControlPoints.GetCount(), "Unexpected event track entry index");
  return m_ControlPoints.GetCount();
}

WInt32 WEventTrack::FindControlPointBefore(WTime x) const
{
  // searches for a control point before OR AT x

  W_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  WInt32 iLowIdx = 0;
  WInt32 iHighIdx = (WInt32)m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (iHighIdx - iLowIdx > 8)
  {
    const WInt32 uiMidIdx = iLowIdx + ((iHighIdx - iLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      iHighIdx = uiMidIdx;
    else
      iLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (WInt32 idx = iHighIdx; idx >= iLowIdx; --idx)
  {
    if (m_ControlPoints[idx].m_Time <= x)
    {
      return idx;
    }
  }

  W_ASSERT_DEBUG(iLowIdx == 0, "Unexpected event track entry index");
  return -1;
}

void WEventTrack::Sample(WTime rangeStart, WTime rangeEnd, WDynamicArray<WHashedString>& out_events) const
{
  if (m_ControlPoints.IsEmpty())
    return;

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  if (rangeStart <= rangeEnd)
  {
    WUInt32 curCpIdx = FindControlPointAfter(rangeStart);

    const WUInt32 uiNumCPs = m_ControlPoints.GetCount();
    while (curCpIdx < uiNumCPs && m_ControlPoints[curCpIdx].m_Time < rangeEnd)
    {
      const WHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      ++curCpIdx;
    }
  }
  else
  {
    WInt32 curCpIdx = FindControlPointBefore(rangeStart);

    while (curCpIdx >= 0 && m_ControlPoints[curCpIdx].m_Time > rangeEnd)
    {
      const WHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      --curCpIdx;
    }
  }
}

void WEventTrack::Save(WStreamWriter& inout_stream) const
{
  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_Events.GetCount();
  for (const WHashedString& name : m_Events)
  {
    inout_stream << name.GetString();
  }

  inout_stream << m_ControlPoints.GetCount();
  for (const ControlPoint& cp : m_ControlPoints)
  {
    inout_stream << cp.m_Time;
    inout_stream << cp.m_uiEvent;
  }
}

void WEventTrack::Load(WStreamReader& inout_stream)
{
  // don't rely on the data being sorted
  m_bSort = true;

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion == 1, "Invalid event track version {0}", uiVersion);

  WUInt32 count = 0;
  WStringBuilder tmp;

  inout_stream >> count;
  m_Events.SetCount(count);
  for (WHashedString& name : m_Events)
  {
    inout_stream >> tmp;
    name.Assign(tmp);
  }

  inout_stream >> count;
  m_ControlPoints.SetCount(count);
  for (ControlPoint& cp : m_ControlPoints)
  {
    inout_stream >> cp.m_Time;
    inout_stream >> cp.m_uiEvent;
  }
}

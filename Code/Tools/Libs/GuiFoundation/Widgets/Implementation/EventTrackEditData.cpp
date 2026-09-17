#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Tracks/EventTrack.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEventTrackControlPointData, 1, WRTTIDefaultAllocator<WEventTrackControlPointData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Tick", m_iTick),
    W_ACCESSOR_PROPERTY("Event", GetEventName, SetEventName),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEventTrackData, 3, WRTTIDefaultAllocator<WEventTrackData>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WEventTrackControlPointData::SetTickFromTime(WTime time, WInt64 iFps)
{
  const WUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (WInt64)WMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

WInt64 WEventTrackData::TickFromTime(WTime time) const
{
  const WUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (WInt64)WMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void WEventTrackData::ConvertToRuntimeData(WEventTrack& out_result) const
{
  out_result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    out_result.AddControlPoint(cp.GetTickAsTime(), cp.m_sEvent);
  }
}

void WEventSet::AddAvailableEvent(WStringView sEvent)
{
  if (sEvent.IsEmpty())
    return;

  if (m_AvailableEvents.Contains(sEvent))
    return;

  m_bModified = true;
  m_AvailableEvents.Insert(sEvent);
}

WResult WEventSet::WriteToDDL(const char* szFile)
{
  WDeferredFileWriter file;
  file.SetOutput(szFile);

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (const auto& s : m_AvailableEvents)
  {
    ddl.BeginObject("Event", s.GetData());
    ddl.EndObject();
  }

  if (file.Close().Succeeded())
  {
    m_bModified = false;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WEventSet::ReadFromDDL(const char* szFile)
{
  m_AvailableEvents.Clear();

  WFileReader file;
  if (file.Open(szFile).Failed())
    return W_FAILURE;

  WOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return W_FAILURE;

  auto* pRoot = ddl.GetRootElement();

  for (auto* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Event"))
    {
      AddAvailableEvent(pChild->GetName());
    }
  }

  m_bModified = false;
  return W_SUCCESS;
}

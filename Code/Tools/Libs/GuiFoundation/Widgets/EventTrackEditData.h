#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WEventTrack;

class W_GUIFOUNDATION_DLL WEventTrackControlPointData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEventTrackControlPointData, WReflectedClass);

public:
  WTime GetTickAsTime() const { return WTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(WTime time, WInt64 iFps);
  const char* GetEventName() const { return m_sEvent.GetData(); }
  void SetEventName(const char* szSz) { m_sEvent.Assign(szSz); }

  WInt64 m_iTick; // 4800 ticks per second
  WHashedString m_sEvent;
};

class W_GUIFOUNDATION_DLL WEventTrackData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEventTrackData, WReflectedClass);

public:
  WInt64 TickFromTime(WTime time) const;
  void ConvertToRuntimeData(WEventTrack& out_result) const;

  WUInt16 m_uiFramesPerSecond = 60;
  WDynamicArray<WEventTrackControlPointData> m_ControlPoints;
};

class W_GUIFOUNDATION_DLL WEventSet
{
public:
  bool IsModified() const { return m_bModified; }

  const WSet<WString>& GetAvailableEvents() const { return m_AvailableEvents; }

  void AddAvailableEvent(WStringView sEvent);

  WResult WriteToDDL(const char* szFile);
  WResult ReadFromDDL(const char* szFile);

private:
  bool m_bModified = false;
  WSet<WString> m_AvailableEvents;
};

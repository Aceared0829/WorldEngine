#pragma once

#include <Foundation/Configuration/CVar.h>

template <typename Type, WCVarType::Enum CVarType>
WTypedCVar<Type, CVarType>::WTypedCVar(WStringView sName, const Type& value, WBitflags<WCVarFlags> flags, WStringView sDescription)
  : WCVar(sName, flags, sDescription)
{
  W_ASSERT_DEBUG(sName.FindSubString(" ") == nullptr, "CVar names must not contain whitespace");

  for (WUInt32 i = 0; i < WCVarValue::ENUM_COUNT; ++i)
    m_Values[i] = value;
}

template <typename Type, WCVarType::Enum CVarType>
WTypedCVar<Type, CVarType>::operator const Type&() const
{
  return (m_Values[WCVarValue::Current]);
}

template <typename Type, WCVarType::Enum CVarType>
WCVarType::Enum WTypedCVar<Type, CVarType>::GetType() const
{
  return CVarType;
}

template <typename Type, WCVarType::Enum CVarType>
void WTypedCVar<Type, CVarType>::SetToDelayedSyncValue()
{
  if (m_Values[WCVarValue::Current] == m_Values[WCVarValue::DelayedSync])
    return;

  // this will NOT trigger a 'restart value changed' event
  m_Values[WCVarValue::Current] = m_Values[WCVarValue::DelayedSync];

  WCVarEvent e(this);
  e.m_EventType = WCVarEvent::ValueChanged;
  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all CVars' event handlers
  s_AllCVarEvents.Broadcast(e);
}

template <typename Type, WCVarType::Enum CVarType>
const Type& WTypedCVar<Type, CVarType>::GetValue(WCVarValue::Enum val) const
{
  return (m_Values[val]);
}

template <typename Type, WCVarType::Enum CVarType>
void WTypedCVar<Type, CVarType>::operator=(const Type& value)
{
  WCVarEvent e(this);

  if (GetFlags().IsAnySet(WCVarFlags::RequiresDelayedSync))
  {
    if (value == m_Values[WCVarValue::DelayedSync]) // no change
      return;

    e.m_EventType = WCVarEvent::DelayedSyncValueChanged;
  }
  else
  {
    if (m_Values[WCVarValue::Current] == value) // no change
      return;

    m_Values[WCVarValue::Current] = value;
    e.m_EventType = WCVarEvent::ValueChanged;
  }

  m_Values[WCVarValue::DelayedSync] = value;

  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}

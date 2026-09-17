#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Threading/ThreadUtils.h>

WInputManager::WEventInput WInputManager::s_InputEvents;
WInputManager::InternalData* WInputManager::s_pData = nullptr;
WString WInputManager::s_sLastCharacters;
bool WInputManager::s_bInputSlotResetRequired = true;
WString WInputManager::s_sExclusiveInputSet;
WMouseCursorDesc WInputManager::s_MouseCursor;
WUInt32 WInputManager::s_uiMouseCursorChangeCounter = 0;
WUInt32 WInputManager::s_uiMouseCursorIdChangeCounter = 0;
WHybridArray<WInputManager::MouseCursorOverride, 4> WInputManager::s_MouseCursorOverrides;
WUInt32 WInputManager::s_uiNextMouseCursorOverrideId = 0;
WUInt32 WInputManager::s_uiHardwareCursorSize = 0;
WUInt32 WInputManager::s_uiUpdateCount = 0;

bool WMouseCursorDesc::operator==(const WMouseCursorDesc& rhs) const
{
  return m_sCursor == rhs.m_sCursor &&
         m_fSize == rhs.m_fSize &&
         m_vHotspot == rhs.m_vHotspot &&
         m_vUvTopLeft == rhs.m_vUvTopLeft &&
         m_vUvBottomRight == rhs.m_vUvBottomRight &&
         m_Rotation == rhs.m_Rotation &&
         m_Color == rhs.m_Color;
}

void WInputManager::SetMouseCursor(const WMouseCursorDesc& desc)
{
  if (s_MouseCursor == desc)
    return;

  const bool bIdentifierChanged = (s_MouseCursor.m_sCursor != desc.m_sCursor);

  s_MouseCursor = desc;
  ++s_uiMouseCursorChangeCounter;

  if (bIdentifierChanged)
  {
    // only whether a custom cursor is used at all can affect the OS cursor,
    // all the other properties are pure visuals
    ++s_uiMouseCursorIdChangeCounter;

    UpdateMouseCursorState();
  }
}

WUInt32 WInputManager::PushMouseCursorOverride(const WMouseCursorOverrideDesc& desc)
{
  W_ASSERT_DEV(WThreadUtils::IsMainThread(), "Mouse cursor overrides may only be modified from the main thread.");

  auto& o = s_MouseCursorOverrides.ExpandAndGetRef();
  o.m_uiId = ++s_uiNextMouseCursorOverrideId;
  o.m_Desc = desc;

  UpdateMouseCursorState();

  return o.m_uiId;
}

void WInputManager::PopMouseCursorOverride(WUInt32 uiOverrideId)
{
  if (uiOverrideId == 0)
    return;

  W_ASSERT_DEV(WThreadUtils::IsMainThread(), "Mouse cursor overrides may only be modified from the main thread.");

  for (WUInt32 i = 0; i < s_MouseCursorOverrides.GetCount(); ++i)
  {
    if (s_MouseCursorOverrides[i].m_uiId == uiOverrideId)
    {
      // must preserve the order of the remaining overrides, the last one wins
      s_MouseCursorOverrides.RemoveAtAndCopy(i);

      UpdateMouseCursorState();
      return;
    }
  }
}

WMouseCursorOverrideDesc WInputManager::GetActiveMouseCursorOverride()
{
  if (!s_MouseCursorOverrides.IsEmpty())
    return s_MouseCursorOverrides.PeekBack().m_Desc;

  WMouseCursorOverrideDesc d;
  d.m_bForceNoClip = false;
  d.m_OSCursor = WMouseCursorOverride::None;

  return d;
}

bool WInputManager::IsCustomMouseCursorActive()
{
  // An explicit override wins over the custom cursor, which in turn wins over what the application wants.
  return !s_MouseCursor.m_sCursor.IsEmpty() && (GetActiveMouseCursorOverride().m_OSCursor != WMouseCursorOverride::ForceOSCursor);
}

void WInputManager::UpdateMouseCursorState()
{
  for (auto pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    if (auto pMouse = WDynamicCast<WInputDeviceMouseKeyboard*>(pDevice))
    {
      pMouse->UpdateEffectiveMouseCursorState();
    }
  }
}

WMouseCursorOverrideRequest::WMouseCursorOverrideRequest(WMouseCursorOverrideRequest&& rhs)
{
  m_uiOverrideId = rhs.m_uiOverrideId;
  m_Desc = rhs.m_Desc;

  rhs.m_uiOverrideId = 0;
}

void WMouseCursorOverrideRequest::operator=(WMouseCursorOverrideRequest&& rhs)
{
  if (this == &rhs)
    return;

  Release();

  m_uiOverrideId = rhs.m_uiOverrideId;
  m_Desc = rhs.m_Desc;

  rhs.m_uiOverrideId = 0;
}

void WMouseCursorOverrideRequest::Request(const WMouseCursorOverrideDesc& desc)
{
  if (IsActive())
  {
    if (m_Desc == desc)
      return;

    Release();
  }

  m_Desc = desc;
  m_uiOverrideId = WInputManager::PushMouseCursorOverride(desc);
}

void WMouseCursorOverrideRequest::Release()
{
  if (!IsActive())
    return;

  WInputManager::PopMouseCursorOverride(m_uiOverrideId);
  m_uiOverrideId = 0;
}

void WInputManager::ClearMouseCursor()
{
  SetMouseCursor(WMouseCursorDesc());
}

WUInt32 WInputManager::GetHardwareCursorSize()
{
  // Update() re-queries this periodically, this only covers being called before the first Update().
  if (s_uiHardwareCursorSize == 0)
  {
    UpdateHardwareCursorSize();
  }

  // fall back to the typical cursor size at 100% scaling, if no device could report one
  return s_uiHardwareCursorSize > 0 ? s_uiHardwareCursorSize : 32;
}

void WInputManager::UpdateHardwareCursorSize()
{
  s_uiHardwareCursorSize = 0;

  if (auto pMouse = GetInputDeviceOfType<WInputDeviceMouseKeyboard>())
  {
    s_uiHardwareCursorSize = pMouse->GetHardwareCursorSize();
  }
}

WInputManager::InternalData& WInputManager::GetInternals()
{
  if (s_pData == nullptr)
    s_pData = W_DEFAULT_NEW(InternalData);

  return *s_pData;
}

void WInputManager::DeallocateInternals()
{
  W_DEFAULT_DELETE(s_pData);
}

WInputManager::WInputSlot::WInputSlot()
{
  m_fValue = 0.0f;
  m_State = WKeyState::Up;
  m_fDeadZone = 0.0f;
}

void WInputManager::RegisterInputSlot(WStringView sInputSlot, WStringView sDefaultDisplayName, WBitflags<WInputSlotFlags> SlotFlags)
{
  WMap<WString, WInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != WInputSlotFlags::Default) && (SlotFlags != WInputSlotFlags::Default))
      {
        WStringBuilder tmp, tmp2;
        tmp.SetPrintf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b",
          sInputSlot.GetData(tmp2), it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());

        WLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // WLog::Debug("Registered Input Slot: '{0}'", sInputSlot);

  WInputSlot& sm = GetInternals().s_InputSlots[sInputSlot];

  sm.m_sDisplayName = sDefaultDisplayName;
  sm.m_SlotFlags = SlotFlags;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

WBitflags<WInputSlotFlags> WInputManager::GetInputSlotFlags(WStringView sInputSlot)
{
  WMap<WString, WInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  WLog::Warning("WInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", sInputSlot);

  return WInputSlotFlags::Default;
}

void WInputManager::SetInputSlotDisplayName(WStringView sInputSlot, WStringView sDefaultDisplayName)
{
  RegisterInputSlot(sInputSlot, sDefaultDisplayName, WInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_sDisplayName = sDefaultDisplayName;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

WStringView WInputManager::GetInputSlotDisplayName(WStringView sInputSlot)
{
  WMap<WString, WInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  WLog::Warning("WInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", sInputSlot);
  return sInputSlot;
}

WStringView WInputManager::GetInputSlotDisplayName(WStringView sInputSet, WStringView sAction, WInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(sInputSet, sAction);

  if (iTrigger < 0)
  {
    for (iTrigger = 0; iTrigger < WInputActionConfig::MaxInputSlotAlternatives; ++iTrigger)
    {
      if (!cfg.m_sInputSlotTrigger[iTrigger].IsEmpty())
        break;
    }
  }

  if (iTrigger >= WInputActionConfig::MaxInputSlotAlternatives)
    return nullptr;

  return GetInputSlotDisplayName(cfg.m_sInputSlotTrigger[iTrigger]);
}

void WInputManager::SetInputSlotDeadZone(WStringView sInputSlot, float fDeadZone)
{
  RegisterInputSlot(sInputSlot, sInputSlot, WInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_fDeadZone = WMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

float WInputManager::GetInputSlotDeadZone(WStringView sInputSlot)
{
  WMap<WString, WInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  WLog::Warning("WInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", sInputSlot);

  WInputSlot s;
  return s.m_fDeadZone; // return the default value
}

WKeyState::Enum WInputManager::GetInputSlotState(WStringView sInputSlot, float* pValue)
{
  WMap<WString, WInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (pValue)
    {
      *pValue = s_bInputSlotResetRequired ? it.Value().m_fValue : it.Value().m_fValueOld;
    }
    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  WLog::Warning("WInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                 "WInputManager::Update before querying device states, or at least call WInputManager::PollHardware.",
    sInputSlot);

  RegisterInputSlot(sInputSlot, sInputSlot, WInputSlotFlags::None);

  return WKeyState::Up;
}

void WInputManager::PollHardware()
{
  if (s_bInputSlotResetRequired)
  {
    s_bInputSlotResetRequired = false;
    ResetInputSlotValues();
  }

  WInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
}

void WInputManager::Update(WTime timeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_sLastCharacters = WInputDevice::RetrieveLastCharactersFromAllDevices();

  UpdateInputActions(timeDifference);

  WInputDevice::ResetAllDevices();

  WInputDevice::UpdateAllHardwareStates(timeDifference);

  // safety net, so that devices that were created after the last state change pick it up as well
  UpdateMouseCursorState();

  // The OS cursor size only changes when the user changes their settings, or when the window moves
  // to a monitor with a different DPI scaling, so re-querying it every few hundred updates is enough.
  if ((s_uiUpdateCount % 250) == 0)
  {
    UpdateHardwareCursorSize();
  }

  ++s_uiUpdateCount;

  s_bInputSlotResetRequired = true;
}

void WInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (WInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValueOld = it.Value().m_fValue;
    it.Value().m_fValue = 0.0f;
  }
}

void WInputManager::GatherDeviceInputSlotValues()
{
  struct AbsValue
  {
    float fOrdinary = 0.0f;
    float fOverride = 0.0f;
    bool bHasOverride = false;
  };

  WMap<WString, AbsValue> absValues(WTempAllocator::Get());

  for (WInputDevice* pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->m_bGeneratedInputRecently = false;

    // iterate over all the input slots that this device provides
    for (auto it = pDevice->m_InputSlotValues.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        WInputManager::WInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the dead-zone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          const bool bAbsolute = Slot.m_SlotFlags.IsSet(WInputSlotFlags::FullAxis) && !Slot.m_SlotFlags.IsSet(WInputSlotFlags::ReportsRelativeValues);

          if (bAbsolute && pDevice->m_bOverridesAbsoluteInput)
          {
            auto& val = absValues[it.Key()];
            val.fOverride = WMath::Max(val.fOverride, it.Value());
            val.bHasOverride = true;
          }
          else if (bAbsolute)
          {
            auto& val = absValues[it.Key()];
            val.fOrdinary = WMath::Max(val.fOrdinary, it.Value());
          }
          else
          {
            Slot.m_fValue = WMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
          }

          // Only count as recent input if the slot represents something the user actively interacted with.
          // This includes buttons/keys (Pressable/Holdable)
          // This excludes passive position tracking (mouse position, touch position) which maintain
          // non-zero values even when no actual user interaction is happening.
          // Mouse move events are also excluded, as they could easily happen accidentally
          if (Slot.m_SlotFlags.IsAnySet(WInputSlotFlags::Pressable | WInputSlotFlags::Holdable))
          {
            pDevice->m_bGeneratedInputRecently = true;
          }
        }
      }
    }
  }

  for (auto it = absValues.GetIterator(); it.IsValid(); ++it)
  {
    auto& val = it.Value();
    if (val.bHasOverride)
    {
      GetInternals().s_InputSlots[it.Key()].m_fValue = val.fOverride;
    }
    else
    {
      GetInternals().s_InputSlots[it.Key()].m_fValue = val.fOrdinary;
    }
  }

  WMap<WString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for (; it.IsValid(); ++it)
  {
    WInputManager::WInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the dead-zone threshold
    if (it.Value() > Slot.m_fDeadZone)
      Slot.m_fValue = WMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
  }

  GetInternals().s_InjectedInputSlots.Clear();
}

void WInputManager::UpdateInputSlotStates()
{
  for (WInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its dead-zone value
    const WKeyState::Enum NewState = WKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);

    if ((it.Value().m_State != NewState) || (NewState != WKeyState::Up))
    {
      it.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputSlotChanged;
      e.m_sInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
  }
}

void WInputManager::RetrieveAllKnownInputSlots(WDynamicArray<WStringView>& out_inputSlots)
{
  out_inputSlots.Clear();
  out_inputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (WInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_inputSlots.PushBack(it.Key().GetData());
  }
}

WString WInputManager::RetrieveLastCharacters(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_sLastCharacters;

  WString sResult = s_sLastCharacters;
  s_sLastCharacters.Clear();
  return sResult;
}

void WInputManager::InjectInputSlotValue(WStringView sInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[sInputSlot] = WMath::Max(GetInternals().s_InjectedInputSlots[sInputSlot], fValue);
}

WStringView WInputManager::GetPressedInputSlot(WInputSlotFlags::Enum mustHaveFlags, WInputSlotFlags::Enum mustNotHaveFlags)
{
  for (WInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != WKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(mustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(mustHaveFlags))
      return it.Key().GetData();
  }

  return WInputSlot_None;
}

WStringView WInputManager::GetInputSlotTouchPoint(WUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return WInputSlot_TouchPoint0;
    case 1:
      return WInputSlot_TouchPoint1;
    case 2:
      return WInputSlot_TouchPoint2;
    case 3:
      return WInputSlot_TouchPoint3;
    case 4:
      return WInputSlot_TouchPoint4;
    case 5:
      return WInputSlot_TouchPoint5;
    case 6:
      return WInputSlot_TouchPoint6;
    case 7:
      return WInputSlot_TouchPoint7;
    case 8:
      return WInputSlot_TouchPoint8;
    case 9:
      return WInputSlot_TouchPoint9;
    default:
      W_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

WStringView WInputManager::GetInputSlotTouchPointPositionX(WUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return WInputSlot_TouchPoint0_PositionX;
    case 1:
      return WInputSlot_TouchPoint1_PositionX;
    case 2:
      return WInputSlot_TouchPoint2_PositionX;
    case 3:
      return WInputSlot_TouchPoint3_PositionX;
    case 4:
      return WInputSlot_TouchPoint4_PositionX;
    case 5:
      return WInputSlot_TouchPoint5_PositionX;
    case 6:
      return WInputSlot_TouchPoint6_PositionX;
    case 7:
      return WInputSlot_TouchPoint7_PositionX;
    case 8:
      return WInputSlot_TouchPoint8_PositionX;
    case 9:
      return WInputSlot_TouchPoint9_PositionX;
    default:
      W_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

WStringView WInputManager::GetInputSlotTouchPointPositionY(WUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return WInputSlot_TouchPoint0_PositionY;
    case 1:
      return WInputSlot_TouchPoint1_PositionY;
    case 2:
      return WInputSlot_TouchPoint2_PositionY;
    case 3:
      return WInputSlot_TouchPoint3_PositionY;
    case 4:
      return WInputSlot_TouchPoint4_PositionY;
    case 5:
      return WInputSlot_TouchPoint5_PositionY;
    case 6:
      return WInputSlot_TouchPoint6_PositionY;
    case 7:
      return WInputSlot_TouchPoint7_PositionY;
    case 8:
      return WInputSlot_TouchPoint8_PositionY;
    case 9:
      return WInputSlot_TouchPoint9_PositionY;
    default:
      W_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

void WInputManager::GetInputDevicesOfType(const WRTTI* pRtti, WDynamicArray<WInputDevice*>& out_devices)
{
  out_devices.Clear();

  for (auto pDev = WInputDevice::GetFirstInstance(); pDev; pDev = pDev->GetNextInstance())
  {
    if (pDev->IsInstanceOf(pRtti))
    {
      out_devices.PushBack(pDev);
    }
  }
}

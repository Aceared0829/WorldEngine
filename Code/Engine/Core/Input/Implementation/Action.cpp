#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

WInputActionConfig::WInputActionConfig()
{
  m_bApplyTimeScaling = true;

  m_fFilterXMinValue = 0.0f;
  m_fFilterXMaxValue = 1.0f;
  m_fFilterYMinValue = 0.0f;
  m_fFilterYMaxValue = 1.0f;

  m_fFilteredPriority = -10000.0f;

  m_OnLeaveArea = LoseFocus;
  m_OnEnterArea = ActivateImmediately;

  for (WInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = WInputSlot_None;
    m_sFilterByInputSlotX[i] = WInputSlot_None;
    m_sFilterByInputSlotY[i] = WInputSlot_None;
  }
}

WInputManager::WActionData::WActionData()
{
  m_fValue = 0.0f;
  m_State = WKeyState::Up;
  m_iTriggeredViaAlternative = -1;
}

void WInputManager::ClearInputMapping(WStringView sInputSet, WStringView sInputSlot)
{
  WActionMap& Actions = GetInternals().s_ActionMapping[sInputSet];

  // iterate over all existing actions
  for (WActionMap::Iterator it = Actions.GetIterator(); it.IsValid(); ++it)
  {
    // iterate over all input slots in the existing action
    for (WUInt32 i1 = 0; i1 < WInputActionConfig::MaxInputSlotAlternatives; ++i1)
    {
      // if that action is triggered by the given input slot, remove that trigger from the action

      if (it.Value().m_Config.m_sInputSlotTrigger[i1] == sInputSlot)
      {
        it.Value().m_Config.m_sInputSlotTrigger[i1].Clear();
      }
    }
  }
}

void WInputManager::SetInputActionConfig(WStringView sInputSet, WStringView sAction, const WInputActionConfig& config, bool bClearPreviousInputMappings)
{
  W_ASSERT_DEV(!sInputSet.IsEmpty(), "The InputSet name must not be empty.");
  W_ASSERT_DEV(!sAction.IsEmpty(), "No input action to map to was given.");

  if (bClearPreviousInputMappings)
  {
    for (WUInt32 i1 = 0; i1 < WInputActionConfig::MaxInputSlotAlternatives; ++i1)
    {
      ClearInputMapping(sInputSet, config.m_sInputSlotTrigger[i1]);
    }
  }

  // store the new action mapping
  WInputManager::WActionData& ad = GetInternals().s_ActionMapping[sInputSet][sAction];
  ad.m_Config = config;

  InputEventData e;
  e.m_EventType = InputEventData::InputActionChanged;
  e.m_sInputSet = sInputSet;
  e.m_sInputAction = sAction;

  s_InputEvents.Broadcast(e);
}

WInputActionConfig WInputManager::GetInputActionConfig(WStringView sInputSet, WStringView sAction)
{
  const WInputSetMap::ConstIterator ItSet = GetInternals().s_ActionMapping.Find(sInputSet);

  if (!ItSet.IsValid())
    return WInputActionConfig();

  const WActionMap::ConstIterator ItAction = ItSet.Value().Find(sAction);

  if (!ItAction.IsValid())
    return WInputActionConfig();

  return ItAction.Value().m_Config;
}

void WInputManager::RemoveInputAction(WStringView sInputSet, WStringView sAction)
{
  GetInternals().s_ActionMapping[sInputSet].Remove(sAction);
}

WKeyState::Enum WInputManager::GetInputActionState(WStringView sInputSet, WStringView sAction, float* pValue, WInt8* pTriggeredSlot)
{
  if (pValue)
    *pValue = 0.0f;

  if (pTriggeredSlot)
    *pTriggeredSlot = -1;

  if (!s_sExclusiveInputSet.IsEmpty() && s_sExclusiveInputSet != sInputSet)
    return WKeyState::Up;

  const WInputSetMap::ConstIterator ItSet = GetInternals().s_ActionMapping.Find(sInputSet);

  if (!ItSet.IsValid())
    return WKeyState::Up;

  const WActionMap::ConstIterator ItAction = ItSet.Value().Find(sAction);

  if (!ItAction.IsValid())
    return WKeyState::Up;

  if (pValue)
    *pValue = ItAction.Value().m_fValue;

  if (pTriggeredSlot)
    *pTriggeredSlot = ItAction.Value().m_iTriggeredViaAlternative;

  return ItAction.Value().m_State;
}

WInputManager::WActionMap::Iterator WInputManager::GetBestAction(WActionMap& Actions, const WString& sSlot, const WActionMap::Iterator& itFirst)
{
  // this function determines which input action should be triggered by the given input slot
  // it will prefer actions with higher priority
  // it will check that all conditions of the action are met (ie. filters like that a mouse cursor is inside a rectangle)
  // if some action had focus before and shall keep it until some key is released, that action will always be preferred

  WActionMap::Iterator ItAction = itFirst;

  WActionMap::Iterator itBestAction;
  float fBestPriority = -1000000;

  if (ItAction.IsValid())
  {
    // take the priority of the last returned value as the basis to compare all other actions against
    fBestPriority = ItAction.Value().m_Config.m_fFilteredPriority;

    // and make sure to skip the last returned action, of course
    ++ItAction;
  }
  else
  {
    // if an invalid iterator is passed in, this is the first call to this function, start searching at the beginning
    ItAction = Actions.GetIterator();
  }

  // check all actions from the given array
  for (; ItAction.IsValid(); ++ItAction)
  {
    WActionData& ThisAction = ItAction.Value();

    WInt8 AltSlot = ThisAction.m_iTriggeredViaAlternative;

    if (AltSlot >= 0)
    {
      if (ThisAction.m_Config.m_sInputSlotTrigger[AltSlot] == sSlot)
        goto hell;
    }
    else
    {
      // if the given slot triggers this action (or any of its alternative slots), continue
      for (AltSlot = 0; AltSlot < WInputActionConfig::MaxInputSlotAlternatives; ++AltSlot)
      {
        if (ThisAction.m_Config.m_sInputSlotTrigger[AltSlot] == sSlot)
          goto hell;
      }
    }

    // if the action is not triggered by this slot, skip it
    continue;

  hell:

    W_ASSERT_DEV(AltSlot >= 0 && AltSlot < WInputActionConfig::MaxInputSlotAlternatives, "Alternate Slot out of bounds.");

    // if the action had input in the last update AND wants to keep the focus, it will ALWAYS get the input, until the input slot gets
    // inactive (key up) independent from priority, overlap of areas etc.
    if (ThisAction.m_State != WKeyState::Up && ThisAction.m_Config.m_OnLeaveArea == WInputActionConfig::KeepFocus)
    {
      // just return this result immediately
      return ItAction;
    }

    // if this action has lower priority than what we already found, ignore it
    if (ThisAction.m_Config.m_fFilteredPriority < fBestPriority)
      continue;

    // if it has the same priority but we already found one, also ignore it
    // if it has the same priority but we did not yet find a 'best action' take this one
    if (ThisAction.m_Config.m_fFilteredPriority == fBestPriority && itBestAction.IsValid())
      continue;

    // this is the "mouse cursor filter" for the x-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotX[AltSlot].IsEmpty())
    {
      const float fVal = GetInternals().s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotX[AltSlot]].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterXMinValue || fVal > ThisAction.m_Config.m_fFilterXMaxValue)
        continue;
    }

    // this is the "mouse cursor filter" for the y-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotY[AltSlot].IsEmpty())
    {
      const float fVal = GetInternals().s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotY[AltSlot]].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterYMinValue || fVal > ThisAction.m_Config.m_fFilterYMaxValue)
        continue;
    }

    // we found something!
    fBestPriority = ThisAction.m_Config.m_fFilteredPriority;
    itBestAction = ItAction;

    ThisAction.m_iTriggeredViaAlternative = AltSlot;
  }

  return itBestAction;
}

void WInputManager::UpdateInputActions(WTime tTimeDifference)
{
  // update each input set
  // all input sets are disjunct from each other, so one key press can have different effects in each input set
  for (WInputSetMap::Iterator ItSets = GetInternals().s_ActionMapping.GetIterator(); ItSets.IsValid(); ++ItSets)
  {
    UpdateInputActions(ItSets.Key().GetData(), ItSets.Value(), tTimeDifference);
  }
}

void WInputManager::UpdateInputActions(WStringView sInputSet, WActionMap& Actions, WTime tTimeDifference)
{
  // reset all action values to zero
  for (WActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
    ItActions.Value().m_fValue = 0.0f;

  // iterate over all input slots and check how their values affect the actions from the current input set
  for (WInputSlotsMap::Iterator ItSlots = GetInternals().s_InputSlots.GetIterator(); ItSlots.IsValid(); ++ItSlots)
  {
    // if this input slot is not active, ignore it; we will reset all actions later
    if (ItSlots.Value().m_fValue == 0.0f)
      continue;

    // If this key got clicked in this frame, it has not been dragged into the active area of the action
    // e.g. the mouse has been clicked while it was inside this area, instead of outside and then moved here
    const bool bFreshClick = ItSlots.Value().m_State == WKeyState::Pressed;

    // find the action that should be affected by this input slot
    WActionMap::Iterator itBestAction;

    // we activate all actions with the same priority simultaneously
    while (true)
    {
      // get the (next) best action
      itBestAction = GetBestAction(Actions, ItSlots.Key(), itBestAction);

      // if we found anything, update its input
      if (!itBestAction.IsValid())
        break;

      const float fSlotScale = itBestAction.Value().m_Config.m_fInputSlotScale[(WUInt32)(itBestAction.Value().m_iTriggeredViaAlternative)];

      float fSlotValue = ItSlots.Value().m_fValue;

      if (fSlotScale >= 0.0f)
        fSlotValue *= fSlotScale;
      else
        fSlotValue = WMath::Pow(fSlotValue, -fSlotScale);

      if ((!ItSlots.Value().m_SlotFlags.IsAnySet(WInputSlotFlags::NeverTimeScale)) && (itBestAction.Value().m_Config.m_bApplyTimeScaling))
        fSlotValue *= (float)tTimeDifference.GetSeconds();

      const float fNewValue = WMath::Max(itBestAction.Value().m_fValue, fSlotValue);

      if (itBestAction.Value().m_Config.m_OnEnterArea == WInputActionConfig::RequireKeyUp)
      {
        // if this action requires that it is only activated by a key press while the mouse is inside it
        // we check whether this is either a fresh click (inside the area) or the action is already active
        // if it is already active, the mouse is most likely held clicked at the moment

        if (bFreshClick || (itBestAction.Value().m_fValue > 0.0f) || (itBestAction.Value().m_State == WKeyState::Pressed) ||
            (itBestAction.Value().m_State == WKeyState::Down))
          itBestAction.Value().m_fValue = fNewValue;
      }
      else
        itBestAction.Value().m_fValue = fNewValue;
    }
  }

  // now update all action states, if any one has not gotten any input from any input slot recently, it will be reset to 'Released' or 'Up'
  for (WActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
  {
    const bool bHasInput = ItActions.Value().m_fValue > 0.0f;
    const WKeyState::Enum NewState = WKeyState::GetNewKeyState(ItActions.Value().m_State, bHasInput);

    if ((NewState != WKeyState::Up) || (NewState != ItActions.Value().m_State))
    {
      ItActions.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputActionChanged;
      e.m_sInputSet = sInputSet;
      e.m_sInputAction = ItActions.Key();

      s_InputEvents.Broadcast(e);
    }

    if (NewState == WKeyState::Up)
      ItActions.Value().m_iTriggeredViaAlternative = -1;
  }
}

void WInputManager::SetActionDisplayName(WStringView sAction, WStringView sDisplayName)
{
  GetInternals().s_ActionDisplayNames[sAction] = sDisplayName;
}

const WString WInputManager::GetActionDisplayName(WStringView sAction)
{
  return GetInternals().s_ActionDisplayNames.GetValueOrDefault(sAction, sAction);
}

void WInputManager::GetAllInputSets(WDynamicArray<WString>& out_inputSetNames)
{
  out_inputSetNames.Clear();

  for (WInputSetMap::Iterator it = GetInternals().s_ActionMapping.GetIterator(); it.IsValid(); ++it)
    out_inputSetNames.PushBack(it.Key());
}

void WInputManager::GetAllInputActions(WStringView sInputSetName, WDynamicArray<WString>& out_inputActions)
{
  const auto& map = GetInternals().s_ActionMapping[sInputSetName];

  out_inputActions.Clear();
  out_inputActions.Reserve(map.GetCount());

  for (WActionMap::ConstIterator it = map.GetIterator(); it.IsValid(); ++it)
    out_inputActions.PushBack(it.Key());
}


#include <RmlUiPlugin/RmlUiInput.h>
#include <Core/Input/InputManager.h>


WRmlUiInputSnapshot WRmlUiInputSnapshot::MakeFromCurrentInput()
{
  WRmlUiInputSnapshot snapshot = WRmlUiInputSnapshot::MakeEmpty();

  const bool bCtrlPressed = WInputManager::GetInputSlotState(WInputSlot_KeyLeftCtrl) >= WKeyState::Pressed ||
    WInputManager::GetInputSlotState(WInputSlot_KeyRightCtrl) >= WKeyState::Pressed;
  const bool bShiftPressed = WInputManager::GetInputSlotState(WInputSlot_KeyLeftShift) >= WKeyState::Pressed ||
    WInputManager::GetInputSlotState(WInputSlot_KeyRightShift) >= WKeyState::Pressed;
  const bool bAltPressed = WInputManager::GetInputSlotState(WInputSlot_KeyLeftAlt) >= WKeyState::Pressed ||
    WInputManager::GetInputSlotState(WInputSlot_KeyRightAlt) >= WKeyState::Pressed;

  if (bCtrlPressed)
    snapshot.m_Buttons |= WRmlUiInputButtons::Ctrl;
  if (bShiftPressed)
    snapshot.m_Buttons |= WRmlUiInputButtons::Shift;
  if (bAltPressed)
    snapshot.m_Buttons |= WRmlUiInputButtons::Alt;

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(WRmlUiInputButtons::s_MouseButtonMappings); ++i)
  {
    WRmlUiInputButtons::MouseButtonMapping mbm = WRmlUiInputButtons::s_MouseButtonMappings[i];
    if (WInputManager::GetInputSlotState(mbm.szEzButton) >= WKeyState::Pressed)
      snapshot.m_Buttons |= mbm.uiEzButton;
  }

  if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelDown) == WKeyState::Pressed)
  {
    snapshot.m_Buttons |= WRmlUiInputButtons::MouseWheelDown;
  }
  if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelUp) == WKeyState::Pressed)
  {
    snapshot.m_Buttons |= WRmlUiInputButtons::MouseWheelUp;
  }

  snapshot.m_sLastCharacters = WInputManager::RetrieveLastCharacters(false);

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(WRmlUiInputButtons::s_KeyMappings); ++i)
  {
    WRmlUiInputButtons::KeyMapping km = WRmlUiInputButtons::s_KeyMappings[i];
    if (WInputManager::GetInputSlotState(km.szEzKey) >= WKeyState::Pressed)
      snapshot.m_Buttons |= km.uiEzKey;
  }

  return snapshot;
}


bool WRmlUiInputProvider::Update(WRmlUiInputSnapshot input)
{
  bool bHasChanged = m_Buttons != input.m_Buttons || m_sLastCharacters != input.m_sLastCharacters;
  m_PrevButtons = m_Buttons;
  m_Buttons = input.m_Buttons;
  m_sLastCharacters = input.m_sLastCharacters;
  return bHasChanged;
}

WKeyState::Enum WRmlUiInputProvider::GetButtonState(WRmlUiInputButtons::Enum button) const
{
  if (m_Buttons.IsSet(button))
  {
    if (!m_PrevButtons.IsSet(button))
      return WKeyState::Pressed;
    return WKeyState::Down;
  }
  else
  {
    if (m_PrevButtons.IsSet(button))
      return WKeyState::Released;
    return WKeyState::Up;
  }
}

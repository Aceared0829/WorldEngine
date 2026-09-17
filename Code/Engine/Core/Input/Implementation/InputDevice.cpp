#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WInputDevice);

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDevice, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WKeyState::Enum WKeyState::GetNewKeyState(WKeyState::Enum prevState, bool bKeyDown)
{
  switch (prevState)
  {
    case WKeyState::Down:
    case WKeyState::Pressed:
      return bKeyDown ? WKeyState::Down : WKeyState::Released;
    case WKeyState::Released:
    case WKeyState::Up:
      return bKeyDown ? WKeyState::Pressed : WKeyState::Up;
  }

  return WKeyState::Up;
}

WInputDevice::WInputDevice()
{
  m_bInitialized = false;
}

void WInputDevice::RegisterInputSlot(WStringView sName, WStringView sDefaultDisplayName, WBitflags<WInputSlotFlags> SlotFlags)
{
  WInputManager::RegisterInputSlot(sName, sDefaultDisplayName, SlotFlags);
}

void WInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  W_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  WLog::Dev("Input Device Type: {0}, Device Name: {1}", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void WInputDevice::UpdateAllHardwareStates(WTime tTimeDifference)
{
  // tell each device to update its hardware
  for (WInputDevice* pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void WInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (WInputDevice* pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->Initialize();
    pDevice->UpdateInputSlotValues();
  }
}

void WInputDevice::ResetAllDevices()
{
  // tell all devices that the input update is through and they might need to reset some values now
  // this is especially important for device types that will get input messages at some undefined time after this call
  // but not during 'UpdateInputSlotValues'
  for (WInputDevice* pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

WString WInputDevice::RetrieveLastCharacters()
{
  WString sResult = m_sLastCharacters;
  m_sLastCharacters.Clear();
  return sResult;
}

WString WInputDevice::RetrieveLastCharactersFromAllDevices()
{
  for (WInputDevice* pDevice = WInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    WString sChars = pDevice->RetrieveLastCharacters();

    if (!sChars.IsEmpty())
      return sChars;
  }

  return WString();
}

float WInputDevice::GetInputSlotState(WStringView sSlot) const
{
  return m_InputSlotValues.GetValueOrDefault(sSlot, 0.f);
}

bool WInputDevice::HasDeviceBeenUsedLastFrame() const
{
  return m_bGeneratedInputRecently;
}

W_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);

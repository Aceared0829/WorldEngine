#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Time/Clock.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceMouseKeyboard, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceController, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WInputDevice* WInputDeviceMouseKeyboard::s_pMouseOver = nullptr;

WInputDeviceController::WInputDeviceController()
{
  for (WInt8 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c] = false;
    m_iPhysicalToVirtualControllerMapping[c] = 0; // by default, map all physical controllers to the first virtual controller
    m_RecentPhysicalControllerInput[c].Clear();

    for (WInt8 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (WUInt8 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void WInputDeviceController::EnableVibration(WUInt8 uiVirtual, bool bEnable)
{
  W_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool WInputDeviceController::IsVibrationEnabled(WUInt8 uiVirtual) const
{
  W_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void WInputDeviceController::SetVibrationStrength(WUInt8 uiVirtual, Motor::Enum motor, float fValue)
{
  W_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  W_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][motor] = WMath::Clamp(fValue, 0.0f, 1.0f);
}

float WInputDeviceController::GetVibrationStrength(WUInt8 uiVirtual, Motor::Enum motor)
{
  W_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  W_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][motor];
}

void WInputDeviceController::SetPhysicalControllerMapping(WUInt8 uiPhysicalController, WInt8 iVirtualController)
{
  W_ASSERT_DEV(uiPhysicalController < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysicalController, MaxControllers);
  W_ASSERT_DEV(iVirtualController < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", iVirtualController, MaxControllers);

  m_iPhysicalToVirtualControllerMapping[uiPhysicalController] = iVirtualController;

  if (iVirtualController < 0)
  {
    WLog::Dev("Input from physical controller {} got deactivated", uiPhysicalController);
  }
  else
  {
    WLog::Dev("Mapped physical controller {} to virtual controller {}", uiPhysicalController, iVirtualController);
  }
}

WInt8 WInputDeviceController::GetPhysicalControllerMapping(WUInt8 uiPhysical) const
{
  W_ASSERT_DEV(uiPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysical, MaxControllers);

  return m_iPhysicalToVirtualControllerMapping[uiPhysical];
}

WBitflags<WPhysicalControllerInput> WInputDeviceController::GetRecentPhysicalControllerInput(WUInt8 uiPhysical) const
{
  W_ASSERT_DEV(uiPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysical, MaxControllers);

  return m_RecentPhysicalControllerInput[uiPhysical];
}

void WInputDeviceController::AddVibrationTrack(WUInt8 uiVirtual, Motor::Enum motor, float* pVibrationTrackValue, WUInt32 uiSamples, float fScalingFactor)
{
  uiSamples = WMath::Min<WUInt32>(uiSamples, MaxVibrationSamples);

  for (WUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][motor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = WMath::Max(fVal, pVibrationTrackValue[s] * fScalingFactor);
    fVal = WMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void WInputDeviceController::UpdateVibration(WTime tTimeDifference)
{
  static WTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const WTime tTimePerSample = WTime::MakeFromSeconds(1.0 / (double)VibrationSamplesPerSecond);

  // advance the vibration track sampling
  while (tElapsedTime >= tTimePerSample)
  {
    tElapsedTime -= tTimePerSample;

    for (WUInt32 c = 0; c < MaxControllers; ++c)
    {
      for (WUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        m_fVibrationTracks[c][m][m_uiVibrationTrackPos] = 0.0f;
    }

    m_uiVibrationTrackPos = (m_uiVibrationTrackPos + 1) % MaxVibrationSamples;
  }

  // we will temporarily store how much vibration is to be applied on each physical controller
  float fVibrationToApply[MaxControllers][Motor::ENUM_COUNT];

  // Initialize with zero (we might not set all values later)
  for (WUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (WUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      fVibrationToApply[c][m] = 0.0f;
    }
  }

  // go through all controllers and motors
  for (WUInt8 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    for (WUInt8 p = 0; p < MaxControllers; ++p)
    {
      if (m_iPhysicalToVirtualControllerMapping[p] == c)
      {
        for (WUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        {
          fVibrationToApply[p][m] = WMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
        }
      }
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (WUInt8 c = 0; c < MaxControllers; ++c)
  {
    for (WUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      ApplyVibration(c, (Motor::Enum)m, fVibrationToApply[c][m]);
    }
  }
}

void WInputDeviceMouseKeyboard::SetShowMouseCursor(bool bShow)
{
  m_bShowMouseCursorDesired = bShow;

  UpdateEffectiveMouseCursorState();
}

void WInputDeviceMouseKeyboard::SetClipMouseCursor(WMouseCursorClipMode::Enum mode)
{
  m_ClipModeDesired = mode;

  UpdateEffectiveMouseCursorState();
}

void WInputDeviceMouseKeyboard::UpdateEffectiveMouseCursorState()
{
  const WMouseCursorOverrideDesc ovr = WInputManager::GetActiveMouseCursorOverride();
  const bool bCustomCursorActive = WInputManager::IsCustomMouseCursorActive();

  // An explicit override wins over the custom cursor, which in turn wins over what the application wants.
  bool bShow;
  switch (ovr.m_OSCursor)
  {
    case WMouseCursorOverride::ForceOSCursor:
      bShow = true;
      break;

    case WMouseCursorOverride::ForceHidden:
      bShow = false;
      break;

    case WMouseCursorOverride::None:
    default:
      bShow = bCustomCursorActive ? false : m_bShowMouseCursorDesired;
      break;
  }

  if (bShow != m_bShowMouseCursorEffective)
  {
    m_bShowMouseCursorEffective = bShow;
    ApplyShowMouseCursor(bShow, bCustomCursorActive);
  }

  const WMouseCursorClipMode::Enum clipMode = ovr.m_bForceNoClip ? WMouseCursorClipMode::NoClip : m_ClipModeDesired;

  if (clipMode != m_ClipModeEffective)
  {
    m_ClipModeEffective = clipMode;
    ApplyClipMouseCursor(clipMode);
  }
}

void WInputDeviceMouseKeyboard::UpdateInputSlotValues()
{
  const char* slots[3] = {WInputSlot_MouseButton0, WInputSlot_MouseButton1, WInputSlot_MouseButton2};
  const char* dlbSlots[3] = {WInputSlot_MouseDblClick0, WInputSlot_MouseDblClick1, WInputSlot_MouseDblClick2};

  const WTime tNow = WClock::GetGlobalClock()->GetLastUpdateTime();

  for (int i = 0; i < 3; ++i)
  {
    m_InputSlotValues[dlbSlots[i]] = 0.0f;

    const bool bDown = m_InputSlotValues[slots[i]] > 0;
    if (bDown)
    {
      if (!m_bMouseDown[i])
      {
        if (tNow - m_LastMouseClick[i] <= m_DoubleClickTime)
        {
          m_InputSlotValues[dlbSlots[i]] = 1.0f;
          m_LastMouseClick[i] = WTime::MakeZero(); // this prevents triple-clicks from appearing as two double clicks
        }
        else
        {
          m_LastMouseClick[i] = tNow;
        }
      }
    }

    m_bMouseDown[i] = bDown;
  }
}

W_STATICLINK_FILE(Core, Core_Input_DeviceTypes_DeviceTypes);

#include <Core/CorePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Time/Clock.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVirtualThumbStick, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WInt32 WVirtualThumbStick::s_iThumbsticks = 0;

WVirtualThumbStick::WVirtualThumbStick()
{
  SetAreaFocusMode(WInputActionConfig::RequireKeyUp, WInputActionConfig::KeepFocus);
  SetTriggerInputSlot(WVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(WVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(WVec2(0.0f), WVec2(0.0f), 0.0f, 0.0f);

  WStringBuilder s;
  s.SetFormat("Thumbstick_{0}", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;
}

WVirtualThumbStick::~WVirtualThumbStick()
{
  WInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void WVirtualThumbStick::SetTriggerInputSlot(WVirtualThumbStick::Input::Enum input, const WInputActionConfig* pCustomConfig)
{
  for (WInt32 i = 0; i < WInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = WInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = WInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i] = WInputSlot_None;
  }

  switch (input)
  {
    case WVirtualThumbStick::Input::Touchpoint:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = WInputSlot_TouchPoint0_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = WInputSlot_TouchPoint0_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = WInputSlot_TouchPoint0;

      m_ActionConfig.m_sFilterByInputSlotX[1] = WInputSlot_TouchPoint1_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[1] = WInputSlot_TouchPoint1_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[1] = WInputSlot_TouchPoint1;

      m_ActionConfig.m_sFilterByInputSlotX[2] = WInputSlot_TouchPoint2_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[2] = WInputSlot_TouchPoint2_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[2] = WInputSlot_TouchPoint2;
    }
    break;
    case WVirtualThumbStick::Input::MousePosition:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = WInputSlot_MousePositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = WInputSlot_MousePositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = WInputSlot_MouseButton0;
    }
    break;
    case WVirtualThumbStick::Input::Custom:
    {
      W_ASSERT_DEV(pCustomConfig != nullptr, "Must pass a custom config, if you want to have a custom config.");

      for (WInt32 i = 0; i < WInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        m_ActionConfig.m_sFilterByInputSlotX[i] = pCustomConfig->m_sFilterByInputSlotX[i];
        m_ActionConfig.m_sFilterByInputSlotY[i] = pCustomConfig->m_sFilterByInputSlotY[i];
        m_ActionConfig.m_sInputSlotTrigger[i] = pCustomConfig->m_sInputSlotTrigger[i];
      }
    }
    break;
  }

  m_bConfigChanged = true;
}

void WVirtualThumbStick::SetThumbstickOutput(WVirtualThumbStick::Output::Enum output, WStringView sOutputLeft, WStringView sOutputRight, WStringView sOutputUp, WStringView sOutputDown)
{
  switch (output)
  {
    case WVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_sOutputLeft = WInputSlot_Controller0_LeftStick_NegX;
      m_sOutputRight = WInputSlot_Controller0_LeftStick_PosX;
      m_sOutputUp = WInputSlot_Controller0_LeftStick_PosY;
      m_sOutputDown = WInputSlot_Controller0_LeftStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_sOutputLeft = WInputSlot_Controller0_RightStick_NegX;
      m_sOutputRight = WInputSlot_Controller0_RightStick_PosX;
      m_sOutputUp = WInputSlot_Controller0_RightStick_PosY;
      m_sOutputDown = WInputSlot_Controller0_RightStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_sOutputLeft = WInputSlot_Controller1_LeftStick_NegX;
      m_sOutputRight = WInputSlot_Controller1_LeftStick_PosX;
      m_sOutputUp = WInputSlot_Controller1_LeftStick_PosY;
      m_sOutputDown = WInputSlot_Controller1_LeftStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_sOutputLeft = WInputSlot_Controller1_RightStick_NegX;
      m_sOutputRight = WInputSlot_Controller1_RightStick_PosX;
      m_sOutputUp = WInputSlot_Controller1_RightStick_PosY;
      m_sOutputDown = WInputSlot_Controller1_RightStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_sOutputLeft = WInputSlot_Controller2_LeftStick_NegX;
      m_sOutputRight = WInputSlot_Controller2_LeftStick_PosX;
      m_sOutputUp = WInputSlot_Controller2_LeftStick_PosY;
      m_sOutputDown = WInputSlot_Controller2_LeftStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_sOutputLeft = WInputSlot_Controller2_RightStick_NegX;
      m_sOutputRight = WInputSlot_Controller2_RightStick_PosX;
      m_sOutputUp = WInputSlot_Controller2_RightStick_PosY;
      m_sOutputDown = WInputSlot_Controller2_RightStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_sOutputLeft = WInputSlot_Controller3_LeftStick_NegX;
      m_sOutputRight = WInputSlot_Controller3_LeftStick_PosX;
      m_sOutputUp = WInputSlot_Controller3_LeftStick_PosY;
      m_sOutputDown = WInputSlot_Controller3_LeftStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_sOutputLeft = WInputSlot_Controller3_RightStick_NegX;
      m_sOutputRight = WInputSlot_Controller3_RightStick_PosX;
      m_sOutputUp = WInputSlot_Controller3_RightStick_PosY;
      m_sOutputDown = WInputSlot_Controller3_RightStick_NegY;
    }
    break;
    case WVirtualThumbStick::Output::Custom:
    {
      m_sOutputLeft = sOutputLeft;
      m_sOutputRight = sOutputRight;
      m_sOutputUp = sOutputUp;
      m_sOutputDown = sOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void WVirtualThumbStick::SetAreaFocusMode(WInputActionConfig::OnEnterArea onEnter, WInputActionConfig::OnLeaveArea onLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = onEnter;
  m_ActionConfig.m_OnLeaveArea = onLeave;
}

void WVirtualThumbStick::SetInputArea(const WVec2& vLowerLeft, const WVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fRadius = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
  m_CenterMode = center;
}

void WVirtualThumbStick::SetFlags(WBitflags<Flags> flags)
{
  m_Flags = flags;
}

void WVirtualThumbStick::SetInputCoordinateAspectRatio(float fWidthDivHeight)
{
  m_fAspectRatio = fWidthDivHeight;
}

void WVirtualThumbStick::GetInputArea(WVec2& out_vLowerLeft, WVec2& out_vUpperRight) const
{
  out_vLowerLeft = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void WVirtualThumbStick::UpdateActionMapping()
{
  if (!m_bConfigChanged)
    return;

  m_ActionConfig.m_fFilterXMinValue = m_vLowerLeft.x;
  m_ActionConfig.m_fFilterXMaxValue = m_vUpperRight.x;
  m_ActionConfig.m_fFilterYMinValue = m_vLowerLeft.y;
  m_ActionConfig.m_fFilterYMaxValue = m_vUpperRight.y;

  WInputManager::SetInputActionConfig(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), m_ActionConfig, false);

  m_bConfigChanged = false;
}

void WVirtualThumbStick::UpdateInputSlotValues()
{
  m_bIsActive = false;

  m_InputSlotValues[m_sOutputLeft] = 0.0f;
  m_InputSlotValues[m_sOutputRight] = 0.0f;
  m_InputSlotValues[m_sOutputUp] = 0.0f;
  m_InputSlotValues[m_sOutputDown] = 0.0f;

  if (!m_bEnabled)
  {
    WInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();

  float fValue;
  WInt8 iTriggerAlt;

  const WKeyState::Enum ks = WInputManager::GetInputActionState(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), &fValue, &iTriggerAlt);

  if (ks != WKeyState::Up)
  {
    m_bIsActive = true;

    if (m_CenterMode == CenterMode::Swipe)
    {
      const WTime tDiff = WClock::GetGlobalClock()->GetTimeDiff();

      m_vCenter = WMath::Lerp(m_vCenter, m_vTouchPos, WMath::Min(1.0f, tDiff.AsFloatInSeconds() * 4.0f));
    }

    m_vTouchPos.Set(0.0f);

    WInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[(WUInt32)iTriggerAlt].GetData(), &m_vTouchPos.x);
    WInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[(WUInt32)iTriggerAlt].GetData(), &m_vTouchPos.y);

    if (ks == WKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
        case CenterMode::InputArea:
          m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
          break;
        case CenterMode::ActivationPoint:
        case CenterMode::Swipe:
          m_vCenter = m_vTouchPos;
          break;
      }
    }

    m_vInputDirection = m_vTouchPos - m_vCenter;

    m_vInputDirection.y /= m_fAspectRatio;

    m_fInputStrength = WMath::Min(m_vInputDirection.GetLength(), m_fRadius) / m_fRadius;
    m_vInputDirection.NormalizeIfNotZero(WVec2::MakeZero()).IgnoreResult();

    const float fThreshold = 0.1f;

    float& l = m_InputSlotValues[m_sOutputLeft];
    float& r = m_InputSlotValues[m_sOutputRight];
    float& u = m_InputSlotValues[m_sOutputUp];
    float& d = m_InputSlotValues[m_sOutputDown];

    if (m_Flags.IsSet(Flags::OnlyMaxAxis))
    {
      const float maxVal = WMath::Max(m_vInputDirection.x, -m_vInputDirection.x, m_vInputDirection.y, -m_vInputDirection.y);

      // only activate the output axis that has the strongest (absolute) value
      if (m_vInputDirection.x == maxVal)
      {
        r = maxVal * m_fInputStrength;
      }
      else if (-m_vInputDirection.x == maxVal)
      {
        l = maxVal * m_fInputStrength;
      }
      else if (m_vInputDirection.y == maxVal)
      {
        d = maxVal * m_fInputStrength;
      }
      else if (-m_vInputDirection.y == maxVal)
      {
        u = maxVal * m_fInputStrength;
      }
    }
    else
    {
      l = WMath::Max(0.0f, -m_vInputDirection.x) * m_fInputStrength;
      r = WMath::Max(0.0f, m_vInputDirection.x) * m_fInputStrength;
      u = WMath::Max(0.0f, -m_vInputDirection.y) * m_fInputStrength;
      d = WMath::Max(0.0f, m_vInputDirection.y) * m_fInputStrength;
    }

    if (l < fThreshold)
      l = 0.0f;
    if (r < fThreshold)
      r = 0.0f;
    if (u < fThreshold)
      u = 0.0f;
    if (d < fThreshold)
      d = 0.0f;
  }
}

void WVirtualThumbStick::RegisterInputSlots()
{
  RegisterInputSlot(WInputSlot_Controller0_LeftStick_NegX, "Left Stick Left", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_LeftStick_PosX, "Left Stick Right", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_LeftStick_NegY, "Left Stick Down", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_LeftStick_PosY, "Left Stick Up", WInputSlotFlags::IsAnalogStick);

  RegisterInputSlot(WInputSlot_Controller0_RightStick_NegX, "Right Stick Left", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_RightStick_PosX, "Right Stick Right", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_RightStick_NegY, "Right Stick Down", WInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(WInputSlot_Controller0_RightStick_PosY, "Right Stick Up", WInputSlotFlags::IsAnalogStick);
}

W_STATICLINK_FILE(Core, Core_Input_Implementation_VirtualThumbStick);

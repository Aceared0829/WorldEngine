#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <GameEngine/Input/XBoxController/InputDeviceXBox.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Time/Clock.h>
#  include <Xinput.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceXBoxController, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WInputDeviceXBoxController::WInputDeviceXBoxController()
{
  for (WInt32 i = 0; i < MaxControllers; ++i)
    m_bControllerConnected[i] = false;
}

WInputDeviceXBoxController::~WInputDeviceXBoxController() = default;

void WInputDeviceXBoxController::RegisterControllerButton(const char* szButton, const char* szName, WBitflags<WInputSlotFlags> SlotFlags)
{
  WStringBuilder s, s2;

  for (WInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    s2.SetFormat("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void WInputDeviceXBoxController::SetDeadZone(const char* szButton)
{
  WStringBuilder s;

  for (WInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    WInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void WInputDeviceXBoxController::RegisterInputSlots()
{
  RegisterControllerButton("button_a", "Button A", WInputSlotFlags::IsButton);
  RegisterControllerButton("button_b", "Button B", WInputSlotFlags::IsButton);
  RegisterControllerButton("button_x", "Button X", WInputSlotFlags::IsButton);
  RegisterControllerButton("button_y", "Button Y", WInputSlotFlags::IsButton);
  RegisterControllerButton("button_start", "Start", WInputSlotFlags::IsButton);
  RegisterControllerButton("button_back", "Back", WInputSlotFlags::IsButton);
  RegisterControllerButton("left_shoulder", "Left Shoulder", WInputSlotFlags::IsButton);
  RegisterControllerButton("right_shoulder", "Right Shoulder", WInputSlotFlags::IsButton);
  RegisterControllerButton("left_trigger", "Left Trigger", WInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("right_trigger", "Right Trigger", WInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("pad_up", "Pad Up", WInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_down", "Pad Down", WInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_left", "Pad Left", WInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_right", "Pad Right", WInputSlotFlags::IsDPad);
  RegisterControllerButton("left_stick", "Left Stick", WInputSlotFlags::IsButton);
  RegisterControllerButton("right_stick", "Right Stick", WInputSlotFlags::IsButton);

  RegisterControllerButton("leftstick_negx", "Left Stick Left", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posx", "Left Stick Right", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_negy", "Left Stick Down", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posy", "Left Stick Up", WInputSlotFlags::IsAnalogStick);

  RegisterControllerButton("rightstick_negx", "Right Stick Left", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posx", "Right Stick Right", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_negy", "Right Stick Down", WInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posy", "Right Stick Up", WInputSlotFlags::IsAnalogStick);

  SetDeadZone("left_trigger");
  SetDeadZone("right_trigger");
  SetDeadZone("leftstick_negx");
  SetDeadZone("leftstick_posx");
  SetDeadZone("leftstick_negy");
  SetDeadZone("leftstick_posy");
  SetDeadZone("rightstick_negx");
  SetDeadZone("rightstick_posx");
  SetDeadZone("rightstick_negy");
  SetDeadZone("rightstick_posy");

  WLog::Success("Initialized XBox 360 Controller.");
}

const WStringView sControllerName[] = {
  "controller0_"_wsv,
  "controller1_"_wsv,
  "controller2_"_wsv,
  "controller3_"_wsv,

  "controller4_"_wsv,
  "controller5_"_wsv,
  "controller6_"_wsv,
  "controller7_"_wsv,
};

static_assert(W_ARRAY_SIZE(sControllerName) >= WInputDeviceController::MaxControllers);

void WInputDeviceXBoxController::SetValue(WInt32 iController, const char* szButton, float fValue)
{
  WStringBuilder s = sControllerName[iController];
  s.Append(szButton);
  float& fVal = m_InputSlotValues[s];
  fVal = WMath::Max(fVal, fValue);
}

void WInputDeviceXBoxController::SetupControllerMappingInOrder()
{
  WTempHybridArray<WUInt8, MaxControllers> connected;
  WTempHybridArray<WUInt8, MaxControllers> disconnected;

  for (WUInt32 uiPhysical = 0; uiPhysical < MaxControllers; ++uiPhysical)
  {
    XINPUT_STATE State;
    if (XInputGetState(uiPhysical, &State) == ERROR_SUCCESS)
    {
      connected.PushBack(uiPhysical);
    }
    else
    {
      disconnected.PushBack(uiPhysical);
    }
  }

  WUInt32 uiVirtual = 0;

  // map the connected controllers to the first virtual controllers
  for (WUInt8 uiPhysical : connected)
  {
    SetPhysicalControllerMapping(uiPhysical, uiVirtual);
    uiVirtual++;
  }

  // then map the disconnected controllers to the remaining virtual controllers
  // so that if they get connected later, they map to a free slot
  for (WUInt8 uiPhysical : disconnected)
  {
    SetPhysicalControllerMapping(uiPhysical, uiVirtual);
    uiVirtual++;
  }
}

void WInputDeviceXBoxController::UpdateHardwareState(WTime tTimeDifference)
{
  UpdateVibration(tTimeDifference);
}

void WInputDeviceXBoxController::UpdateInputSlotValues()
{
  // reset all keys
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  XINPUT_STATE State[MaxControllers];
  bool bIsAvailable[MaxControllers];

  // update not connected controllers only every few milliseconds, apparently it takes quite some time to do this
  // even on not connected controllers
  static WTime tLastControllerSearch;
  static WUInt32 uiControllerSearch = 0;
  const WTime tNow = WClock::GetGlobalClock()->GetLastUpdateTime();
  const bool bSearchControllers = tNow - tLastControllerSearch > WTime::MakeFromSeconds(0.5);

  if (bSearchControllers)
  {
    uiControllerSearch = (uiControllerSearch + 1) % MaxControllers;
    tLastControllerSearch = tNow;
  }

  // get the data from all physical devices
  for (WUInt32 uiPhysical = 0; uiPhysical < MaxControllers; ++uiPhysical)
  {
    if (m_bControllerConnected[uiPhysical] || (bSearchControllers && uiPhysical == uiControllerSearch))
    {
      bIsAvailable[uiPhysical] = (XInputGetState(uiPhysical, &State[uiPhysical]) == ERROR_SUCCESS);

      if (m_bControllerConnected[uiPhysical] != bIsAvailable[uiPhysical])
      {
        WLog::Info("XBox Controller {0} has been {1}.", uiPhysical, bIsAvailable[uiPhysical] ? "connected" : "disconnected");

        // this makes sure to reset all values below
        if (!bIsAvailable[uiPhysical])
          WMemoryUtils::ZeroFill(&State[uiPhysical], 1);
      }
    }
    else
      bIsAvailable[uiPhysical] = m_bControllerConnected[uiPhysical];
  }

  // check from which physical device to take the input data
  for (WUInt8 uiPhysical = 0; uiPhysical < MaxControllers; ++uiPhysical)
  {
    m_RecentPhysicalControllerInput[uiPhysical].Clear();

    // if the controller is not active, no point in updating it
    // if it just got inactive, this will reset it once, because the state is only passed on after this loop
    if (!m_bControllerConnected[uiPhysical])
      continue;

    WInt8 iVirtual = GetPhysicalControllerMapping(uiPhysical);
    if (iVirtual < 0)
      continue;

    SetValue(iVirtual, "pad_up", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_down", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_left", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_right", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_start", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_back", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "left_stick", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "right_stick", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "left_shoulder", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "right_shoulder", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_a", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_b", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_x", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_y", ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) ? 1.0f : 0.0f);

    const float fTriggerRange = 255.0f;

    SetValue(iVirtual, "left_trigger", State[uiPhysical].Gamepad.bLeftTrigger / fTriggerRange);
    SetValue(iVirtual, "right_trigger", State[uiPhysical].Gamepad.bRightTrigger / fTriggerRange);

    // all input points have dead-zones, so we can let the state handler do the rest
    SetValue(iVirtual, "leftstick_negx", (State[uiPhysical].Gamepad.sThumbLX < 0) ? (-State[uiPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_posx", (State[uiPhysical].Gamepad.sThumbLX > 0) ? (State[uiPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_negy", (State[uiPhysical].Gamepad.sThumbLY < 0) ? (-State[uiPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_posy", (State[uiPhysical].Gamepad.sThumbLY > 0) ? (State[uiPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);

    SetValue(iVirtual, "rightstick_negx", (State[uiPhysical].Gamepad.sThumbRX < 0) ? (-State[uiPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_posx", (State[uiPhysical].Gamepad.sThumbRX > 0) ? (State[uiPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_negy", (State[uiPhysical].Gamepad.sThumbRY < 0) ? (-State[uiPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_posy", (State[uiPhysical].Gamepad.sThumbRY > 0) ? (State[uiPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);

    if ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0)
    {
      m_RecentPhysicalControllerInput[uiPhysical].Add(WPhysicalControllerInput::Start);
    }

    if ((State[uiPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0)
    {
      m_RecentPhysicalControllerInput[uiPhysical].Add(WPhysicalControllerInput::Back);
    }

    if ((State[uiPhysical].Gamepad.wButtons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y)) != 0)
    {
      m_RecentPhysicalControllerInput[uiPhysical].Add(WPhysicalControllerInput::FrontButton);
    }

    if ((State[uiPhysical].Gamepad.wButtons & (XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER)) != 0)
    {
      m_RecentPhysicalControllerInput[uiPhysical].Add(WPhysicalControllerInput::ShoulderButton);
    }
  }


  for (WUInt32 uiPhysical = 0; uiPhysical < MaxControllers; ++uiPhysical)
  {
    m_bControllerConnected[uiPhysical] = bIsAvailable[uiPhysical];
  }
}

bool WInputDeviceXBoxController::IsPhysicalControllerConnected(WUInt8 uiPhysical) const
{
  W_ASSERT_DEV(uiPhysical < MaxControllers, "Invalid Controller Index {0}", uiPhysical);

  return m_bControllerConnected[uiPhysical];
}

void WInputDeviceXBoxController::ApplyVibration(WUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  if (!m_bControllerConnected[uiPhysicalController])
    return;

  static XINPUT_VIBRATION v[MaxControllers];

  if (eMotor == Motor::LeftMotor)
    v[uiPhysicalController].wLeftMotorSpeed = (WORD)(fStrength * 65535.0f);

  if (eMotor == Motor::RightMotor)
  {
    v[uiPhysicalController].wRightMotorSpeed = (WORD)(fStrength * 65535.0f);

    XInputSetState(uiPhysicalController, &v[uiPhysicalController]);
  }
}

static WInputDeviceXBoxController* g_InputDeviceXBoxController = nullptr;

WInputDeviceXBoxController* WInputDeviceXBoxController::GetDevice()
{
  if (g_InputDeviceXBoxController == nullptr)
  {
    g_InputDeviceXBoxController = W_DEFAULT_NEW(WInputDeviceXBoxController);
  }

  return g_InputDeviceXBoxController;
}


// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceXBoxController)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager"
    
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    W_DEFAULT_DELETE(g_InputDeviceXBoxController);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WInputDeviceXBoxController::GetDevice();
  }
 
  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    W_DEFAULT_DELETE(g_InputDeviceXBoxController);
  }
 
W_END_SUBSYSTEM_DECLARATION;
// clang-format on

#endif


W_STATICLINK_FILE(GameEngine, GameEngine_Input_XBoxController_InputDeviceXBox);

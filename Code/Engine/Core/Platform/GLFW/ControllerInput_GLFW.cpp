#include <Core/CorePCH.h>

#if W_ENABLED(W_SUPPORTS_GLFW)

#  include <Core/Input/DeviceTypes/Controller.h>
#  include <Core/Input/InputManager.h>
#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Types/UniquePtr.h>

#  include <GLFW/glfw3.h>

class WControllerInputGlfw : public WInputDeviceController
{
public:
  virtual void InitializeDevice() override;

  virtual void UpdateInputSlotValues() override;

  virtual void ResetInputSlotValues() override;

  virtual void RegisterInputSlots() override;

  virtual bool IsPhysicalControllerConnected(WUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(WUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  void RegisterControllerButton(const char* szButton, const char* szName, WBitflags<WInputSlotFlags> SlotFlags);
  void SetDeadZone(const char* szButton);
  void SetControllerValue(WStringBuilder& tmp, WUInt8 controllerIndex, const char* inputSlotName, float value);

  bool m_bInitialized = false;
};

namespace
{
  WUniquePtr<WControllerInputGlfw> g_pControllerInputGlfw;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Core, ControllerInput)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "InputManager",
    "Window"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pControllerInputGlfw = W_DEFAULT_NEW(WControllerInputGlfw);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pControllerInputGlfw.Clear();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  struct ControllerButtonMapping
  {
    const char* WName;
    int glfwIndex;
  };

  const ControllerButtonMapping g_ControllerButtonMappings[] = {
    {"button_a", GLFW_GAMEPAD_BUTTON_A},
    {"button_b", GLFW_GAMEPAD_BUTTON_B},
    {"button_x", GLFW_GAMEPAD_BUTTON_X},
    {"button_y", GLFW_GAMEPAD_BUTTON_Y},
    {"button_start", GLFW_GAMEPAD_BUTTON_START},
    {"button_back", GLFW_GAMEPAD_BUTTON_BACK},
    {"left_shoulder", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER},
    {"right_shoulder", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"pad_up", GLFW_GAMEPAD_BUTTON_DPAD_UP},
    {"pad_down", GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
    {"pad_left", GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
    {"pad_right", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"left_stick", GLFW_GAMEPAD_BUTTON_LEFT_THUMB},
    {"right_stick", GLFW_GAMEPAD_BUTTON_RIGHT_THUMB}};
} // namespace

void WControllerInputGlfw::RegisterControllerButton(const char* szButton, const char* szName, WBitflags<WInputSlotFlags> SlotFlags)
{
  WStringBuilder s, s2;

  for (WInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    s2.SetFormat("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void WControllerInputGlfw::SetDeadZone(const char* szButton)
{
  WStringBuilder s;

  for (WInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    WInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void WControllerInputGlfw::SetControllerValue(WStringBuilder& tmp, WUInt8 controllerIndex, const char* inputSlotName, float value)
{
  tmp.SetFormat("controller{0}_{1}", controllerIndex, inputSlotName);
  m_InputSlotValues[tmp] = value;
}

void WControllerInputGlfw::InitializeDevice()
{
  // Make a arbitrary call into glfw so that we can check if the library is properly initialized
  glfwJoystickPresent(0);

  // Check for errors during the previous call
  const char* desc;
  int errorCode = glfwGetError(&desc);
  if (errorCode != GLFW_NO_ERROR)
  {
    WLog::Warning("glfw joystick and gamepad input not avaiable: {} - {}", errorCode, desc);
    return;
  }
  m_bInitialized = true;
}

void WControllerInputGlfw::UpdateInputSlotValues()
{
  if (!m_bInitialized)
  {
    return;
  }

  WStringBuilder inputSlotName;

  // update all virtual controllers
  for (WUInt8 uiVirtual = 0; uiVirtual < MaxControllers; ++uiVirtual)
  {
    // check from which physical device to take the input data
    const WInt8 iPhysical = GetPhysicalControllerMapping(uiVirtual);

    // if the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    int glfwId = GLFW_JOYSTICK_1 + iPhysical;
    if (glfwJoystickPresent(glfwId))
    {
      if (glfwJoystickIsGamepad(glfwId))
      {
        GLFWgamepadstate state = {};
        if (glfwGetGamepadState(glfwId, &state))
        {
          for (size_t buttonIndex = 0; buttonIndex < W_ARRAY_SIZE(g_ControllerButtonMappings); buttonIndex++)
          {
            const ControllerButtonMapping mapping = g_ControllerButtonMappings[buttonIndex];
            SetControllerValue(inputSlotName, uiVirtual, mapping.WName, (state.buttons[mapping.glfwIndex] == GLFW_PRESS) ? 1.0f : 0.0f);
          }

          SetControllerValue(inputSlotName, uiVirtual, "left_trigger", state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
          SetControllerValue(inputSlotName, uiVirtual, "right_trigger", state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);

          SetControllerValue(inputSlotName, uiVirtual, "leftstick_negx", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] * -1.0f));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_posx", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_negy", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_posy", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * -1.0f));

          SetControllerValue(inputSlotName, uiVirtual, "rightstick_negx", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * -1.0f));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_posx", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_negy", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_posy", WMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * -1.0f));
        }
      }
    }
  }
}

void WControllerInputGlfw::ResetInputSlotValues()
{
}

void WControllerInputGlfw::RegisterInputSlots()
{
  if (!m_bInitialized)
  {
    return;
  }

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
}

bool WControllerInputGlfw::IsPhysicalControllerConnected(WUInt8 uiPhysical) const
{
  if (!m_bInitialized)
  {
    return false;
  }

  int glfwId = GLFW_JOYSTICK_1 + uiPhysical;
  return glfwJoystickPresent(glfwId) && glfwJoystickIsGamepad(glfwId);
}

void WControllerInputGlfw::ApplyVibration(WUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  // Unfortunately GLFW does not have vibration support
}

#endif


W_STATICLINK_FILE(Core, Core_Platform_GLFW_ControllerInput_GLFW);

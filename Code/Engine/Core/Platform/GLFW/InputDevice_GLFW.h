#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

#if W_ENABLED(W_SUPPORTS_GLFW)

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

class W_CORE_DLL WInputDeviceMouseKeyboard_GLFW : public WInputDeviceMouseKeyboard
{
  W_ADD_DYNAMIC_REFLECTION(WInputDeviceMouseKeyboard_GLFW, WInputDeviceMouseKeyboard);

public:
  WInputDeviceMouseKeyboard_GLFW(GLFWwindow* windowHandle);
  ~WInputDeviceMouseKeyboard_GLFW();

  virtual WUInt32 GetHardwareCursorSize() const override;

  // GLFW callback for key pressed, released, repeated events
  void OnKey(int key, int scancode, int action, int mods);

  // GLFW callback for text input (each UTF32 code point individually)
  void OnCharacter(unsigned int codepoint);

  // GLFW callback on mouse move
  void OnCursorPosition(double xpos, double ypos);

  // GLFW callback on mouse button actions
  void OnMouseButton(int button, int action, int mods);

  // GLFW callback for mouse scroll
  void OnScroll(double xoffset, double yoffset);

private:
  virtual void ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive) override;
  virtual void ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode) override;

  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

private:
  GLFWwindow* m_pWindow = nullptr;
  WVec2d m_LastPos = WVec2d(WMath::MaxValue<double>());
};

#endif

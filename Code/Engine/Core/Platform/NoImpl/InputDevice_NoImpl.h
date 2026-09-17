#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class W_CORE_DLL WInputDeviceMouseKeyboard_NoImpl : public WInputDeviceMouseKeyboard
{
  W_ADD_DYNAMIC_REFLECTION(WInputDeviceMouseKeyboard_NoImpl, WInputDeviceMouseKeyboard);

public:
  WInputDeviceMouseKeyboard_NoImpl(WUInt32 uiWindowNumber);
  ~WInputDeviceMouseKeyboard_NoImpl();

private:
  virtual void ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive) override;
  virtual void ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode) override;

  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};

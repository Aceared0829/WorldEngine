#pragma once

#include <Core/Input/InputDevice.h>

struct WAndroidInputEvent;
struct AInputEvent;

/// Android standard input device.
class W_CORE_DLL WInputDevice_Android : public WInputDevice
{
  W_ADD_DYNAMIC_REFLECTION(WInputDevice_Android, WInputDevice);

public:
  WInputDevice_Android();
  ~WInputDevice_Android();

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void AndroidInputEventHandler(WAndroidInputEvent& event);
  void AndroidAppCommandEventHandler(WInt32 iCmd);
  bool AndroidHandleInput(AInputEvent* pEvent);

private:
  WInt32 m_iResolutionX = 0;
  WInt32 m_iResolutionY = 0;
};

#pragma once

#include <Core/Input/DeviceTypes/Controller.h>
#include <GameEngine/GameEngineDLL.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

/// An implementation of WInputDeviceController that handles XBox controllers.
///
/// Works on all platforms that provide the XINPUT API.
class W_GAMEENGINE_DLL WInputDeviceXBoxController : public WInputDeviceController
{
  W_ADD_DYNAMIC_REFLECTION(WInputDeviceXBoxController, WInputDeviceController);

public:
  WInputDeviceXBoxController();
  ~WInputDeviceXBoxController();

  /// Returns an WInputDeviceXBoxController device.
  static WInputDeviceXBoxController* GetDevice();
  virtual bool IsPhysicalControllerConnected(WUInt8 uiPhysical) const override;

  /// Maps connected controllers to virtual controllers in the order of which ones are connected.
  ///
  /// So usually 0->0, 1->1, etc.
  /// But if for instance controller 0 is not connected, it would be 1->0, 2->1, 3->2, 0->3
  ///
  /// By default all controllers map to virtual controller 0, so 0->0, 1->0, 2->0, 3->0.
  void SetupControllerMappingInOrder();

private:
  static void RegisterControllerButton(const char* szButton, const char* szName, WBitflags<WInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);

  virtual void ApplyVibration(WUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;
  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(WTime tTimeDifference) override;

  void SetValue(WInt32 iController, const char* szButton, float fValue);

  bool m_bControllerConnected[MaxControllers];
};

#endif

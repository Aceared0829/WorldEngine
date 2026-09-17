#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/Declarations.h>

#define WInputSlot_XR_Hand_Left_Trigger "xr_hand_left_trigger"
#define WInputSlot_XR_Hand_Left_Select_Click "xr_hand_left_select_click"
#define WInputSlot_XR_Hand_Left_Menu_Click "xr_hand_left_menu_click"
#define WInputSlot_XR_Hand_Left_Squeeze_Click "xr_hand_left_squeeze_click"

#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_NegX "xr_hand_left_primary_analog_stick_negx"
#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_PosX "xr_hand_left_primary_analog_stick_posx"
#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_NegY "xr_hand_left_primary_analog_stick_negy"
#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_PosY "xr_hand_left_primary_analog_stick_posy"
#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_Click "xr_hand_left_primary_analog_stick_click"
#define WInputSlot_XR_Hand_Left_Primary_Analog_Stick_Touch "xr_hand_left_primary_analog_stick_touch"

#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_NegX "xr_hand_left_secondary_analog_stick_negx"
#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_PosX "xr_hand_left_secondary_analog_stick_posx"
#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_NegY "xr_hand_left_secondary_analog_stick_negy"
#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_PosY "xr_hand_left_secondary_analog_stick_posy"
#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_Click "xr_hand_left_secondary_analog_stick_click"
#define WInputSlot_XR_Hand_Left_Secondary_Analog_Stick_Touch "xr_hand_left_secondary_analog_stick_touch"


#define WInputSlot_XR_Hand_Right_Trigger "xr_hand_right_trigger"
#define WInputSlot_XR_Hand_Right_Select_Click "xr_hand_right_select_click"
#define WInputSlot_XR_Hand_Right_Menu_Click "xr_hand_right_menu_click"
#define WInputSlot_XR_Hand_Right_Squeeze_Click "xr_hand_right_squeeze_click"

#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_NegX "xr_hand_right_primary_analog_stick_negx"
#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_PosX "xr_hand_right_primary_analog_stick_posx"
#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_NegY "xr_hand_right_primary_analog_stick_negy"
#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_PosY "xr_hand_right_primary_analog_stick_posy"
#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_Click "xr_hand_right_primary_analog_stick_click"
#define WInputSlot_XR_Hand_Right_Primary_Analog_Stick_Touch "xr_hand_right_primary_analog_stick_touch"

#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_NegX "xr_hand_right_secondary_analog_stick_negx"
#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_PosX "xr_hand_right_secondary_analog_stick_posx"
#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_NegY "xr_hand_right_secondary_analog_stick_negy"
#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_PosY "xr_hand_right_secondary_analog_stick_posy"
#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_Click "xr_hand_right_secondary_analog_stick_click"
#define WInputSlot_XR_Hand_Right_Secondary_Analog_Stick_Touch "xr_hand_right_secondary_analog_stick_touch"


class W_GAMEENGINE_DLL WXRInputDevice : public WInputDevice
{
  W_ADD_DYNAMIC_REFLECTION(WXRInputDevice, WInputDevice);

public:
  /// \name Devices
  ///@{

  /// Fills out a list of valid (connected) device IDs.
  virtual void GetDeviceList(WHybridArray<WXRDeviceID, 64>& out_devices) const = 0;
  /// Returns the deviceID for a specific type of device.
  /// If the device is not connected, -1 is returned instead.
  virtual WXRDeviceID GetDeviceIDByType(WXRDeviceType::Enum type) const = 0;
  /// Returns the current device state for a valid device ID.
  virtual const WXRDeviceState& GetDeviceState(WXRDeviceID deviceID) const = 0;
  /// Returns the device name for a valid device ID.
  ///
  /// This returns a human readable name to identify the device.
  /// For WXRDeviceType::HMD the name is always 'HMD'.
  /// This can be used for e.g. controllers to create custom game input logic
  /// or mappings if a certain type of controller is used.
  /// Values could be for example:
  /// 'Simple Controller', 'Mixed Reality Motion Controller', 'Hand Interaction' etc.
  virtual WString GetDeviceName(WXRDeviceID deviceID) const = 0;
  /// Returns the device features for a valid device ID.
  virtual WBitflags<WXRDeviceFeatures> GetDeviceFeatures(WXRDeviceID deviceID) const = 0;

  /// Returns the input event. Allows tracking device addition and removal.
  const WXRDeviceEvent& GetInputEvent() { return m_InputEvents; }

  ///@}

protected:
  WXRDeviceEvent m_InputEvents;
};

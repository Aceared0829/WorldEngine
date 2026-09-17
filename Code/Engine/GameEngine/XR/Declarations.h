#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>

struct WHMDInfo
{
  WString m_sDeviceName;
  WString m_sDeviceDriver;
  WSizeU32 m_vEyeRenderTargetSize;
};

/// Defines the stage space used for the XR experience.
///
/// This value is set by the WStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct WXRStageSpace
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WXRStageSpace);

struct WXRTransformSpace
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Local,  ///< Sets the local transform to the pose in stage space. Use if owner is direct child of WStageSpaceComponent.
    Global, ///< Uses the global transform of the device in world space.
    Default = Local,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WXRTransformSpace);

struct WXRDeviceType
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    HMD,
    LeftController,
    RightController,
    DeviceID0,
    DeviceID1,
    DeviceID2,
    DeviceID3,
    DeviceID4,
    DeviceID5,
    DeviceID6,
    DeviceID7,
    DeviceID8,
    DeviceID9,
    DeviceID10,
    DeviceID11,
    DeviceID12,
    DeviceID13,
    DeviceID14,
    DeviceID15,
    Default = HMD,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WXRDeviceType);

using WXRDeviceID = WInt8;

/// A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the WStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct W_GAMEENGINE_DLL WXRDeviceState
{
  WXRDeviceState();

  WVec3 m_vGripPosition;
  WQuat m_qGripRotation;

  WVec3 m_vAimPosition;
  WQuat m_qAimRotation;

  WEnum<WXRDeviceType> m_Type;
  bool m_bGripPoseIsValid = false;
  bool m_bAimPoseIsValid = false;
  bool m_bDeviceIsConnected = false;
};

/// Defines features the given device supports.
struct WXRDeviceFeatures
{
  using StorageType = WUInt32;
  enum Enum : WUInt32
  {
    None = 0,
    Trigger = W_BIT(0),                   ///< Float input. Analog trigger value 0-1.
    Select = W_BIT(1),                    ///< Bool input. Trigger fully pressed.
    Menu = W_BIT(2),                      ///< Bool input. Menu/Start button.
    Squeeze = W_BIT(3),                   ///< Float input. Analog squeeze/grip value 0-1.
    PrimaryAnalogStick = W_BIT(4),        ///< 2D axis input. Thumbstick/joystick.
    PrimaryAnalogStickClick = W_BIT(5),   ///< Bool input. Thumbstick pressed.
    PrimaryAnalogStickTouch = W_BIT(6),   ///< Bool input. Thumbstick touched.
    SecondaryAnalogStick = W_BIT(7),      ///< 2D axis input. Trackpad or secondary joystick.
    SecondaryAnalogStickClick = W_BIT(8), ///< Bool input. Trackpad/secondary joystick pressed.
    SecondaryAnalogStickTouch = W_BIT(9), ///< Bool input. Trackpad/secondary joystick touched.
    PrimaryButton = W_BIT(10),            ///< Bool input. Primary face button (X/A).
    PrimaryButtonTouch = W_BIT(11),       ///< Bool input. Primary face button touched.
    SecondaryButton = W_BIT(12),          ///< Bool input. Secondary face button (Y/B).
    SecondaryButtonTouch = W_BIT(13),     ///< Bool input. Secondary face button touched.
    GripPose = W_BIT(14),                 ///< 3D Pose input. Grip/hand position.
    AimPose = W_BIT(15),                  ///< 3D Pose input. Aim/pointer direction.
    Default = None
  };

  struct Bits
  {
    StorageType Trigger : 1;
    StorageType Select : 1;
    StorageType Menu : 1;
    StorageType Squeeze : 1;
    StorageType PrimaryAnalogStick : 1;
    StorageType PrimaryAnalogStickClick : 1;
    StorageType PrimaryAnalogStickTouch : 1;
    StorageType SecondaryAnalogStick : 1;
    StorageType SecondaryAnalogStickClick : 1;
    StorageType SecondaryAnalogStickTouch : 1;
    StorageType PrimaryButton : 1;
    StorageType PrimaryButtonTouch : 1;
    StorageType SecondaryButton : 1;
    StorageType SecondaryButtonTouch : 1;
    StorageType GripPose : 1;
    StorageType AimPose : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WXRDeviceFeatures);


struct WXRDeviceEventData
{
  enum class Type : WUInt8
  {
    DeviceAdded,
    DeviceRemoved,
  };

  Type m_Type;
  WXRDeviceID uiDeviceID = 0;
};

using WXRDeviceEvent = WEvent<const WXRDeviceEventData&>;

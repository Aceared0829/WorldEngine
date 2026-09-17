#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WXRTransformSpace, 1)
  W_BITFLAGS_CONSTANTS(WXRTransformSpace::Local, WXRTransformSpace::Global)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WXRDeviceType, 1)
  W_BITFLAGS_CONSTANTS(WXRDeviceType::HMD, WXRDeviceType::LeftController, WXRDeviceType::RightController)
  W_BITFLAGS_CONSTANTS(WXRDeviceType::DeviceID0, WXRDeviceType::DeviceID1, WXRDeviceType::DeviceID2, WXRDeviceType::DeviceID3)
  W_BITFLAGS_CONSTANTS(WXRDeviceType::DeviceID4, WXRDeviceType::DeviceID5, WXRDeviceType::DeviceID6, WXRDeviceType::DeviceID7)
  W_BITFLAGS_CONSTANTS(WXRDeviceType::DeviceID8, WXRDeviceType::DeviceID9, WXRDeviceType::DeviceID10, WXRDeviceType::DeviceID11)
  W_BITFLAGS_CONSTANTS(WXRDeviceType::DeviceID12, WXRDeviceType::DeviceID13, WXRDeviceType::DeviceID14, WXRDeviceType::DeviceID15)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WXRDeviceState::WXRDeviceState()
{
  m_vGripPosition.SetZero();
  m_qGripRotation.SetIdentity();

  m_vAimPosition.SetZero();
  m_qAimRotation.SetIdentity();
}


W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_Declaration);

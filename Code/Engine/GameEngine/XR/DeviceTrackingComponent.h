#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

struct WXRPoseLocation
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Grip,
    Aim,
    Default = Grip,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WXRPoseLocation);

//////////////////////////////////////////////////////////////////////////


using WDeviceTrackingComponentManager = WComponentManagerSimple<class WDeviceTrackingComponent, WComponentUpdateType::WhenSimulating>;

/// Tracks the position of a XR device and applies it to the owner.
class W_GAMEENGINE_DLL WDeviceTrackingComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WDeviceTrackingComponent, WComponent, WDeviceTrackingComponentManager);

public:
  WDeviceTrackingComponent();
  ~WDeviceTrackingComponent();

  /// Sets the type of device this component is going to track.
  void SetDeviceType(WEnum<WXRDeviceType> type);
  WEnum<WXRDeviceType> GetDeviceType() const;

  void SetPoseLocation(WEnum<WXRPoseLocation> poseLocation);
  WEnum<WXRPoseLocation> GetPoseLocation() const;

  /// Whether to set the owner's local or global transform, see WXRTransformSpace.
  void SetTransformSpace(WEnum<WXRTransformSpace> space);
  WEnum<WXRTransformSpace> GetTransformSpace() const;

  //
  // WComponent Interface
  //

protected:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

protected:
  void Update();

  WEnum<WXRDeviceType> m_DeviceType;
  WEnum<WXRPoseLocation> m_PoseLocation;
  WEnum<WXRTransformSpace> m_Space;
  bool m_bRotation = true;
  bool m_bScale = true;
};

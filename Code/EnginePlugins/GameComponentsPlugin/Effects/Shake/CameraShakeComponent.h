#pragma once

#include <Core/World/ComponentManager.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

using WCameraShakeComponentManager = WComponentManagerSimple<class WCameraShakeComponent, WComponentUpdateType::WhenSimulating>;

/// This component is used to apply a shaking effect to the game object that it is attached to.
///
/// The shake is applied as a local rotation around the Y and Z axis, assuming the camera is looking along the positive X axis.
/// The component can be attached to the same object as a camera component,
/// but it is usually best to insert a dedicated shake object as a parent of the camera.
///
/// How much shake to apply is controlled through the m_MinShake and m_MaxShake properties.
///
/// The shake values can be modified dynamically to force a shake, but it is more convenient to instead place shake volumes (see WCameraShakeVolumeComponent and derived classes). The camera shake component samples these volumes using its own location and uses the
/// determined strength to fade between its min and max shake amount.
///
/// \see WCameraShakeVolumeComponent
class W_GAMECOMPONENTS_DLL WCameraShakeComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WCameraShakeComponent, WComponent, WCameraShakeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WCameraShakeComponent

  /// How much shake to apply as the minimum value, even if no shake volume is found or the shake strength is zero.
  WAngle m_MinShake; // [ property ]

  /// How much shake to apply at shake strength 1.
  WAngle m_MaxShake = WAngle::MakeFromDegree(5); // [ property ]

public:
  WCameraShakeComponent();
  ~WCameraShakeComponent();

protected:
  void Update();

  void GenerateKeyframe();
  float GetStrengthAtPosition() const;

  float m_fLastStrength = 0.0f;
  WTime m_ReferenceTime;
  WAngle m_Rotation;
  WQuat m_qPrevTarget = WQuat::MakeIdentity();
  WQuat m_qNextTarget = WQuat::MakeIdentity();
};

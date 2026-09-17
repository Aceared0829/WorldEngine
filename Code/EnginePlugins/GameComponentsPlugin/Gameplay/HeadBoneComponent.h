#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/ComponentManager.h>

using WHeadBoneComponentManager = WComponentManagerSimple<class WHeadBoneComponent, WComponentUpdateType::WhenSimulating>;

/// Applies a vertical rotation in local space (local Y axis) to the owner game object.
///
/// This component is meant to be used to apply a vertical rotation to a camera.
/// For first-person camera movement, typically the horizontal rotation is already taken care of
/// through the rotation of a character controller.
/// To additionally allow a limited vertical rotation, this component is introduced.
/// It is assumed that a local rotation of zero represents the "forward" camera direction and the camera is allowed
/// to rotate both up and down by a certain number of degrees, for example 80 degrees.
/// This component takes care to apply that amount of rotation and not more.
///
/// Call SetVerticalRotation() or ChangeVerticalRotation() to set or add some rotation.
class W_GAMECOMPONENTS_DLL WHeadBoneComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WHeadBoneComponent, WComponent, WHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WHeadBoneComponent

public:
  WHeadBoneComponent();
  ~WHeadBoneComponent();

  /// Sets the vertical rotation to a fixed value.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void SetVerticalRotation(float fRadians); // [ scriptable ]

  /// Adds or subtracts from the current rotation.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void ChangeVerticalRotation(float fRadians);                 // [ scriptable ]

  WAngle m_MaxVerticalRotation = WAngle::MakeFromDegree(80); // [ property ]

protected:
  void Update();

  WAngle m_NewVerticalRotation;
  WAngle m_CurVerticalRotation;
};

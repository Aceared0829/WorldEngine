#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using WRotorComponentManager = WComponentManagerSimple<class WRotorComponent, WComponentUpdateType::WhenSimulating>;

/// Applies a rotation to the game object that it is attached to.
///
/// The rotation may be endless, or limited to a certain amount of rotation.
/// It may also automatically turn around and accelerate and decelerate.
class W_GAMEENGINE_DLL WRotorComponent : public WTransformComponent
{
  W_DECLARE_COMPONENT_TYPE(WRotorComponent, WTransformComponent, WRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WRotorComponent

public:
  WRotorComponent();
  ~WRotorComponent();

  /// How much to rotate before reaching the end and either stopping or turning around.
  /// Set to zero for endless rotation.
  WInt32 m_iDegreeToRotate = 0; // [ property ]

  /// The acceleration to reach the target speed.
  float m_fAcceleration = 1.0f; // [ property ]

  /// The deceleration to brake to zero speed before reaching the end rotation.
  float m_fDeceleration = 1.0f; // [ property ]

  /// The axis around which to rotate. In local space of the game object.
  WEnum<WBasisAxis> m_Axis = WBasisAxis::PositiveZ; // [ property ]

  /// How much the rotation axis may randomly deviate to not have all objects rotate the same way.
  WAngle m_AxisDeviation; // [ property ]

protected:
  void Update();

  WVec3 m_vRotationAxis = WVec3(0, 0, 1);
  WQuat m_qLastRotation = WQuat::MakeIdentity();
};

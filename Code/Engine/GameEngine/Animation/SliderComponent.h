#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using WSliderComponentManager = WComponentManagerSimple<class WSliderComponent, WComponentUpdateType::WhenSimulating>;

/// Applies a sliding transform to the game object that it is attached to.
///
/// The object is moved along a local axis either once or back and forth.
class W_GAMEENGINE_DLL WSliderComponent : public WTransformComponent
{
  W_DECLARE_COMPONENT_TYPE(WSliderComponent, WTransformComponent, WSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WSliderComponent

public:
  WSliderComponent();
  ~WSliderComponent();

  /// How far to move the object along the axis before reaching the end point.
  float m_fDistanceToTravel = 1.0f; // [ property ]

  /// The acceleration to use to reach the target speed.
  float m_fAcceleration = 0.0f; // [ property ]

  /// The deceleration to use to brake to zero speed before reaching the end.
  float m_fDeceleration = 0.0; // [ property ]

  /// The axis along which to move the object.
  WEnum<WBasisAxis> m_Axis = WBasisAxis::PositiveZ; // [ property ]

  /// If non-zero, the slider starts at a random offset as if it had already been moving for up to this amount of time.
  WTime m_RandomStart; // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};

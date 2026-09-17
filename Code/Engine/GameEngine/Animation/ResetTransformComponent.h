#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using WResetTransformComponentManager = WComponentManager<class WResetTransformComponent, WBlockStorageType::Compact>;

/// This component sets the local transform of its owner to known values when the simulation starts.
///
/// This component is meant for use cases where an object may be activated and deactivated over and over.
/// For example due to a state machine switching between different object states by (de-)activating a sub-tree of objects.
///
/// Every time an object becomes active, it may want to start moving again from a fixed location.
/// This component helps with that, by reseting the local transform of its owner to such a fixed location once.
///
/// After that, it does nothing else, until it gets deactivated and reactivated again.
class W_GAMEENGINE_DLL WResetTransformComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WResetTransformComponent, WComponent, WResetTransformComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WResetTransformComponent

public:
  WResetTransformComponent();
  ~WResetTransformComponent();

  WVec3 m_vLocalPosition = WVec3::MakeZero();
  WQuat m_qLocalRotation = WQuat::MakeIdentity();
  WVec3 m_vLocalScaling = WVec3(1, 1, 1);
  float m_fLocalUniformScaling = 1.0f;

  bool m_bResetLocalPositionX = true;
  bool m_bResetLocalPositionY = true;
  bool m_bResetLocalPositionZ = true;
  bool m_bResetLocalRotation = true;
  bool m_bResetLocalScaling = true;
};

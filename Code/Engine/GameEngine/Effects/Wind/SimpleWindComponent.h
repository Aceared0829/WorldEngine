#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using WSimpleWindComponentManager = WComponentManagerSimple<class WSimpleWindComponent, WComponentUpdateType::WhenSimulating>;

/// Calculates one global wind force using a very basic formula.
///
/// This component computes a wind vector that varies between a minimum and maximum strength
/// and around a certain direction.
///
/// Sets up the WSimpleWindWorldModule as the implementation of the WWindWorldModuleInterface.
///
/// When sampling the wind through this interface, the returned value is the same at every location.
///
/// Use a single instance of this component in a scene, when you need wind values, e.g. to make cloth and ropes sway,
/// but don't need a complex wind simulation.
class W_GAMEENGINE_DLL WSimpleWindComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSimpleWindComponent, WComponent, WSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WSimpleWindComponent

public:
  WSimpleWindComponent();
  ~WSimpleWindComponent();

  /// The minimum speed that the wind should always blow with.
  WEnum<WWindStrength> m_MinWindStrength; // [ property ]

  /// The maximum speed that the wind should blow with.
  WEnum<WWindStrength> m_MaxWindStrength; // [ property ]

  /// The wind blows in the positive X direction of the game object.
  /// The direction may deviate this much from that direction. Set to 180 degree to remove the limit.
  WAngle m_Deviation; // [ property ]

protected:
  void Update();
  void ComputeNextState();

  float m_fLastStrength = 0;
  float m_fNextStrength = 0;
  WVec3 m_vLastDirection;
  WVec3 m_vNextDirection;
  WTime m_LastChange;
  WTime m_NextChange;
};

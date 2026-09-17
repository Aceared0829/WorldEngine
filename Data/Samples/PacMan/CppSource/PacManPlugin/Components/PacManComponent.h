#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <PacManPlugin/PacManPluginDLL.h>

using PacManComponentManager = WComponentManagerSimple<class PacManComponent, WComponentUpdateType::WhenSimulating>;

// The component that handles PacMan's behavior (movement / interaction with ghosts and coins)
class PacManComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(PacManComponent, WComponent, PacManComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // PacManComponent

public:
  PacManComponent();
  ~PacManComponent();

private:
  // Called once every frame.
  void Update();

  // Message handler for input messages that the input component sends to us once per frame.
  void OnMsgInputActionTriggered(WMsgInputActionTriggered& msg);
  // Message handler for trigger messages, that the triggers on PacMan send to us whenever PacMan overlaps with a coin or a ghost.
  void OnMsgTriggerTriggered(WMsgTriggerTriggered& msg);

  // the direction into which PacMan currently travels (0 = +X, 1 = +Y, 2 = -X, 3 = -Y)
  WalkDirection m_Direction = WalkDirection::Up;
  // the direction into which PacMan is supposed to travel when possible
  WalkDirection m_TargetDirection = WalkDirection::Up;

  // the prefab that we spawn every time we collect a coin (plays a sound and such)
  WPrefabResourceHandle m_hCollectCoinEffect;
  // the prefab that we spawn when PacMan dies (particle effect, sound)
  WPrefabResourceHandle m_hLoseGameEffect;

  WSharedPtr<WBlackboard> m_pStateBlackboard;
};

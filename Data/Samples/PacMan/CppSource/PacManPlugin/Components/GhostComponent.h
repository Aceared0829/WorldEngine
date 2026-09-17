#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <PacManPlugin/PacManPluginDLL.h>

using GhostComponentManager = WComponentManagerSimple<class GhostComponent, WComponentUpdateType::WhenSimulating>;

class GhostComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(GhostComponent, WComponent, GhostComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // GhostComponent

public:
  GhostComponent();
  ~GhostComponent();

private:
  void Update();

  WalkDirection m_Direction = WalkDirection::Up;

  WPrefabResourceHandle m_hDisappear;

  WSharedPtr<WBlackboard> m_pStateBlackboard;

  float m_fSpeed = 2.0f;
};

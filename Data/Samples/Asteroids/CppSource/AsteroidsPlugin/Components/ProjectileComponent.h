#pragma once

#include <Core/World/World.h>

class ProjectileComponent;
using ProjectileComponentManager = WComponentManagerSimple<ProjectileComponent, WComponentUpdateType::WhenSimulating>;

class ProjectileComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(ProjectileComponent, WComponent, ProjectileComponentManager);

public:
  ProjectileComponent();
  void Update();

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

  WTime m_TimeToLive;
  float m_fSpeed = 0.0f;
  WInt32 m_iBelongsToPlayer = -1;
  float m_fDoesDamage = 0.0f;
};

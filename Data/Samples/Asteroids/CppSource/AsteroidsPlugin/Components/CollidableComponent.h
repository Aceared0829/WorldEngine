#pragma once

#include <Core/World/World.h>

class CollidableComponent;
using CollidableComponentManager = WComponentManager<CollidableComponent, WBlockStorageType::FreeList>;

class CollidableComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(CollidableComponent, WComponent, CollidableComponentManager);

public:
  CollidableComponent();

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

  float m_fCollisionRadius;
};

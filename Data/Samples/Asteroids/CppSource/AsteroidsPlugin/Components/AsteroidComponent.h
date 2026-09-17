#pragma once

#include <Core/World/World.h>

class AsteroidComponent;
using AsteroidComponentManager = WComponentManagerSimple<AsteroidComponent, WComponentUpdateType::WhenSimulating>;

class AsteroidComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(AsteroidComponent, WComponent, AsteroidComponentManager);

public:
  AsteroidComponent();

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

  void Update();

  float m_fRotationSpeed;

protected:
  virtual void OnSimulationStarted() override;
};

#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>

using SampleBounceComponentManager = WComponentManagerSimple<class SampleBounceComponent, WComponentUpdateType::WhenSimulating>;

class SampleBounceComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(SampleBounceComponent, WComponent, SampleBounceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SampleBounceComponent

public:
  SampleBounceComponent();
  ~SampleBounceComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;                     // [ property ]
  WAngle m_Speed = WAngle::MakeFromDegree(90); // [ property ]
};

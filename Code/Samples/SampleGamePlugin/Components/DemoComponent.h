#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-manager
using DemoComponentManager = WComponentManagerSimple<class DemoComponent, WComponentUpdateType::WhenSimulating>;
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-class
class DemoComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(DemoComponent, WComponent, DemoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DemoComponent

public:
  DemoComponent();
  ~DemoComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;                     // [ property ]
  WAngle m_Speed = WAngle::MakeFromDegree(90); // [ property ]
};
// END-DOCS-CODE-SNIPPET

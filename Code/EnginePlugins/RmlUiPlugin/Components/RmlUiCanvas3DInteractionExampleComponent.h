#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RmlUiPlugin/RmlUiInput.h>

using WRmlUiCanvas3DInteractionComponentManager = WComponentManagerSimple<class WRmlUiCanvas3DInteractionExampleComponent, WComponentUpdateType::WhenSimulating, WBlockStorageType::Compact, WWorldUpdatePhase::PostTransform>;

class WCpuMeshResource;
class WPhysicsWorldModuleInterface;

class W_RMLUIPLUGIN_DLL WRmlUiCanvas3DInteractionExampleComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WRmlUiCanvas3DInteractionExampleComponent, WComponent, WRmlUiCanvas3DInteractionComponentManager);

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

public:
  WRmlUiCanvas3DInteractionExampleComponent();
  ~WRmlUiCanvas3DInteractionExampleComponent();

  void Update();
  void Interact(WRmlUiInputSnapshot input);

private:
  
  WUInt32 m_uiCollisionLayer = 0;
  float m_fMaxDistance = 2.0f;
  WPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};

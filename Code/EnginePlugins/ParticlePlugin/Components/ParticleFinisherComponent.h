#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct WMsgExtractRenderData;
struct WMsgInterruptPlaying;

class W_PARTICLEPLUGIN_DLL WParticleFinisherComponentManager final : public WComponentManager<class WParticleFinisherComponent, WBlockStorageType::Compact>
{
  using SUPER = WComponentManager<class WParticleFinisherComponent, WBlockStorageType::Compact>;

public:
  WParticleFinisherComponentManager(WWorld* pWorld);

  void UpdateBounds();
};

/// Automatically created by the particle system to finish playing a particle effect.
///
/// This is needed to play a particle effect to the end, when a game object with a particle effect on it gets deleted.
/// This component should never be instantiated manually.
class W_PARTICLEPLUGIN_DLL WParticleFinisherComponent final : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WParticleFinisherComponent, WRenderComponent, WParticleFinisherComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WParticleFinisherComponent

public:
  WParticleFinisherComponent();
  ~WParticleFinisherComponent();

  /// Forwards to InterruptEffect().
  void OnMsgInterruptPlaying(WMsgInterruptPlaying& ref_msg); // [ msg handler ]

  WParticleEffectController m_EffectController;

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  void UpdateBounds();
};

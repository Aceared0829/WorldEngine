#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that applies a color gradient to particles
///
/// The gradient can be sampled based on particle lifetime or speed.
/// The final color is multiplied by the tint color.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_ColorGradient final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_ColorGradient, WParticleBehaviorFactory);

public:
  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  // ************************************* PROPERTIES ***********************************

  WEnum<WParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  WColor m_TintColor = WColor::White;
  bool m_bApplyAlpha = true;
  WEnum<WGradientSource> m_GradientSource;
  WColorGradient m_Gradient;
  WColorGradientResourceHandle m_hSharedGradient;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_ColorGradient final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_ColorGradient, WParticleBehavior);

public:
  const WColorGradient* m_pGradient = nullptr;
  WEnum<WParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  WColor m_TintColor;
  bool m_bApplyAlpha = true;

  virtual void CreateRequiredStreams() override;

protected:
  friend class WParticleBehaviorFactory_ColorGradient;

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
  WColor m_InitColor;

  /// Staggered update: which particle index to update this frame (cycles from 0 to m_uiCurrentUpdateInterval-1)
  WUInt8 m_uiFirstToUpdate = 0;
  /// Staggered update: only update every N-th particle per frame to reduce cost
  WUInt8 m_uiCurrentUpdateInterval = 8;
};

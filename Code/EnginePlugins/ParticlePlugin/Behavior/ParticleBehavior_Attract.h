#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Pulls particles toward a point with distance-based falloff.
///
/// Can modify velocity (physically correct acceleration) or position directly (snapping).
/// MinDistance prevents singularity at the attractor center.
///
/// When \a Target is set to \a Attractors, the behavior queries the spatial system for nearby
/// WParticleAttractorComponents and applies each one's contribution per particle. The attractor
/// list is refreshed roughly once per second for performance.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Attract final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Attract, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Attract();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fInfluence = 5.0f;
  bool m_bAffectVelocity = true;

  /// Maximum number of nearby attractors to consider. Higher values are more expensive.
  WUInt8 m_uiMaxAttractors = 1;
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior_Attract final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Attract, WParticleBehavior);

public:
  float m_fInfluence = 5.0f;
  bool m_bAffectVelocity = true;

  WUInt8 m_uiMaxAttractors = 1;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
};

#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// Behavior that pulls particles along when the effect moves
///
/// Useful for effects attached to moving objects where particles should
/// follow the movement to some degree rather than staying in world space.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_PullAlong final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_PullAlong, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_PullAlong();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fStrength; ///< How much particles follow the effect movement (0-1 range)
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_PullAlong final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_PullAlong, WParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fStrength = 0.5;

protected:
  virtual void Process(WUInt64 uiNumElements) override;
  virtual void StepParticleSystem(const WTime& tDiff, WUInt32 uiNumNewParticles) override;

  bool m_bFirstTime = true;
  WVec3 m_vLastEmitterPosition;
  WVec3 m_vApplyPull;
  WProcessingStream* m_pStreamPosition;
};

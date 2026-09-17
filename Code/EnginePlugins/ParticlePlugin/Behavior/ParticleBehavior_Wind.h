#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WWindWorldModuleInterface;

/// Behavior that applies wind forces to particles
///
/// Reads wind samples from the world and applies them to particle positions.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Wind final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Wind, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Wind();
  ~WParticleBehaviorFactory_Wind();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fWindInfluence = 1.0f;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Wind final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Wind, WParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fWindInfluence = 1.0f;

protected:
  friend class WParticleBehaviorFactory_Wind;

  virtual void Process(WUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  WProcessingStream* m_pStreamPosition = nullptr;
};

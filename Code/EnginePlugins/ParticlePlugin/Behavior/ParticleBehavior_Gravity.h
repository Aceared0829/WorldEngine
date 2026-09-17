#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// Behavior that applies gravity to particles
///
/// Uses the world's gravity vector from the physics module.
/// The gravity factor scales the applied gravity force.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Gravity final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Gravity, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Gravity();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  float m_fGravityFactor;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Gravity final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Gravity, WParticleBehavior);

public:
  float m_fGravityFactor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class WParticleBehaviorFactory_Gravity;

  virtual void Process(WUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  WPhysicsWorldModuleInterface* m_pPhysicsModule;

  WProcessingStream* m_pStreamVelocity;
};

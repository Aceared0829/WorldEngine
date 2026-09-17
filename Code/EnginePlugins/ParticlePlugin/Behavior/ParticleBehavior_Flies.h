#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that makes particles move like flies or insects
///
/// Particles move at a constant speed with random direction changes.
/// They tend to stay within a certain distance from the effect origin.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Flies final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Flies, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Flies();
  ~WParticleBehaviorFactory_Flies();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fSpeed = 0.2f;                                    ///< Movement speed in units per second
  float m_fPathLength = 0.2f;                               ///< Distance traveled before changing direction
  float m_fMaxEmitterDistance = 0.5f;                       ///< Maximum distance from effect origin before turning back
  WAngle m_MaxSteeringAngle = WAngle::MakeFromDegree(30); ///< Maximum angle change per direction update
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Flies final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Flies, WParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSpeed = 0.2f;
  float m_fPathLength = 0.2f;
  float m_fMaxEmitterDistance = 0.5f;
  WAngle m_MaxSteeringAngle = WAngle::MakeFromDegree(30);

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;

  WTime m_TimeToChangeDir;
};

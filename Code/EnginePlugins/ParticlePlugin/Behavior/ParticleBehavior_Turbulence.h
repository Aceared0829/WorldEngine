#pragma once

#include <Foundation/SimdMath/SimdNoise.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Applies a turbulent force to particles using 3D curl noise.
///
/// Curl noise produces divergence-free flow fields that look natural and avoid
/// compression artifacts. Useful for smoke, fire, and magical effects.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Turbulence final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Turbulence, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Turbulence();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fStrength = 2.0f;
  float m_fFrequency = 1.0f;
  WVec3 m_vScrollSpeed = WVec3::MakeZero();
  WUInt8 m_uiOctaves = 1;
  bool m_bAffectVelocity = true;
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior_Turbulence final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Turbulence, WParticleBehavior);

public:
  float m_fStrength = 2.0f;
  float m_fFrequency = 1.0f;
  WVec3 m_vScrollSpeed = WVec3::MakeZero();
  WUInt8 m_uiOctaves = 1;
  bool m_bAffectVelocity = true;

protected:
  virtual void OnFinalize() override;

  virtual void CreateRequiredStreams() override;
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;

  WSimdPerlinNoise m_Noise;
  WTime m_TotalTime;
};

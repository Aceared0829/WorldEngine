#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns particles based on distance traveled
///
/// Spawns particles when the effect moves a certain distance from the last spawn position.
/// Useful for creating trails or footstep effects.
class W_PARTICLEPLUGIN_DLL WParticleEmitterFactory_Distance final : public WParticleEmitterFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitterFactory_Distance, WParticleEmitterFactory);

public:
  WParticleEmitterFactory_Distance();

  virtual const WRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(WParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  float m_fDistanceThreshold = 0.1f;    ///< Distance that must be traveled before spawning
  WUInt32 m_uiSpawnCountMin = 1;       ///< Minimum particles per spawn
  WUInt32 m_uiSpawnCountRange = 0;     ///< Random range added to spawn count
  WString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count
};


class W_PARTICLEPLUGIN_DLL WParticleEmitter_Distance final : public WParticleEmitter
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitter_Distance, WParticleEmitter);

public:
  float m_fDistanceThresholdSQR;
  WUInt32 m_uiSpawnCountMin;
  WUInt32 m_uiSpawnCountRange;
  WTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual bool IsContinuous() const override;
  virtual void OnFinalize() override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  virtual WParticleEmitterState IsFinished() override;
  virtual WUInt32 ComputeSpawnCount(const WTime& tDiff) override;

  bool m_bFirstUpdate = true;
  WVec3 m_vLastSpawnPosition;
};

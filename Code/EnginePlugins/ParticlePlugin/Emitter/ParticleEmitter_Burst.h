#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns a burst of particles over a short duration
///
/// Spawns all particles distributed over the duration time.
/// After the burst completes, the emitter becomes inactive.
class W_PARTICLEPLUGIN_DLL WParticleEmitterFactory_Burst final : public WParticleEmitterFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitterFactory_Burst, WParticleEmitterFactory);

public:
  WParticleEmitterFactory_Burst();

  virtual const WRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(WParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  WTime m_Duration;                    ///< Duration over which to distribute the burst (0 = single frame)
  WTime m_StartDelay;                  ///< Delay before burst starts

  WUInt32 m_uiSpawnCountMin;           ///< Minimum number of particles to spawn
  WUInt32 m_uiSpawnCountRange;         ///< Random range added to spawn count
  WString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count
};


class W_PARTICLEPLUGIN_DLL WParticleEmitter_Burst final : public WParticleEmitter
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitter_Burst, WParticleEmitter);

public:
  WTime m_Duration;   // overall duration in which the emitter is considered active, 0 for single frame
  WTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  WUInt32 m_uiSpawnCountMin;
  WUInt32 m_uiSpawnCountRange;
  WTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void OnFinalize() override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}

  virtual WParticleEmitterState IsFinished() override;
  virtual WUInt32 ComputeSpawnCount(const WTime& tDiff) override;

  WUInt32 m_uiSpawnCountLeft = 0;
  float m_fSpawnPerSecond = 0;
  float m_fSpawnAccu = 0;
};

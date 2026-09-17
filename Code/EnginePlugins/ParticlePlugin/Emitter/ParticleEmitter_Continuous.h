#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

using WCurve1DResourceHandle = WTypedResourceHandle<class WCurve1DResource>;

/// Emitter that continuously spawns particles over time
///
/// Spawn rate can be constant or modulated by a curve.
/// Continues emitting until the effect is stopped.
class W_PARTICLEPLUGIN_DLL WParticleEmitterFactory_Continuous final : public WParticleEmitterFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitterFactory_Continuous, WParticleEmitterFactory);

public:
  WParticleEmitterFactory_Continuous();

  virtual const WRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(WParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  WTime m_StartDelay;                   ///< Delay before emission starts

  WUInt32 m_uiSpawnCountPerSec;         ///< Base spawn rate per second
  WUInt32 m_uiSpawnCountPerSecRange;    ///< Random range added to spawn rate
  WString m_sSpawnCountScaleParameter;  ///< Optional parameter to scale spawn rate

  WCurve1DResourceHandle m_hCountCurve; ///< Optional curve to modulate spawn rate
  WTime m_CurveDuration;                ///< Duration for curve evaluation
};


class W_PARTICLEPLUGIN_DLL WParticleEmitter_Continuous final : public WParticleEmitter
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitter_Continuous, WParticleEmitter);

public:
  WTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  WUInt32 m_uiSpawnCountPerSec;
  WUInt32 m_uiSpawnCountPerSecRange;
  WTempHashedString m_sSpawnCountScaleParameter;

  WCurve1DResourceHandle m_hCountCurve;
  WTime m_CurveDuration;


  virtual void CreateRequiredStreams() override {}

protected:
  virtual bool IsContinuous() const override { return true; }

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}
  virtual void OnFinalize() override;

  virtual WParticleEmitterState IsFinished() override;
  virtual WUInt32 ComputeSpawnCount(const WTime& tDiff) override;

  WTime m_CountCurveTime;
  WTime m_TimeSinceRandom;
  float m_fCurSpawnPerSec;
  float m_fCurSpawnCounter;
};

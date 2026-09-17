#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

/// Emitter that spawns particles in response to events
///
/// Only spawns when the specified event is raised.
/// Useful for creating impact effects or other event-driven particles.
class W_PARTICLEPLUGIN_DLL WParticleEmitterFactory_OnEvent final : public WParticleEmitterFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitterFactory_OnEvent, WParticleEmitterFactory);

public:
  WParticleEmitterFactory_OnEvent();
  ~WParticleEmitterFactory_OnEvent();

  virtual const WRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(WParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WString m_sEventName;                ///< Name of the event that triggers emission
  WUInt32 m_uiSpawnCountMin = 1;       ///< Minimum particles per event
  WUInt32 m_uiSpawnCountRange = 0;     ///< Random range added to spawn count
  WString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count
};

class W_PARTICLEPLUGIN_DLL WParticleEmitter_OnEvent final : public WParticleEmitter
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitter_OnEvent, WParticleEmitter);

public:
  WTempHashedString m_sEventName;
  WUInt32 m_uiSpawnCountMin = 1;
  WUInt32 m_uiSpawnCountRange = 0;
  WTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}

  virtual WParticleEmitterState IsFinished() override;
  virtual WUInt32 ComputeSpawnCount(const WTime& tDiff) override;

  virtual void ProcessEventQueue(WParticleEventQueue queue) override;

  bool m_bSpawn = false;
};

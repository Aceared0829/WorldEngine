#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WParticleSystemInstance;
class WProcessingStream;
class WParticleEmitter;

/// Base class for all particle emitters
class W_PARTICLEPLUGIN_DLL WParticleEmitterFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitterFactory, WReflectedClass);

public:
  virtual const WRTTI* GetEmitterType() const = 0;
  virtual void CopyEmitterProperties(WParticleEmitter* pEmitter, bool bFirstTime) const = 0;

  WParticleEmitter* CreateEmitter(WParticleSystemInstance* pOwner) const;
  virtual void QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const = 0;

  virtual void Save(WStreamWriter& inout_stream) const = 0;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) = 0;
};

/// Current state of a particle emitter
enum class WParticleEmitterState
{
  Active,       ///< Emitter is actively spawning particles
  Finished,     ///< Emitter will not spawn more particles
  OnlyReacting, ///< Only spawns on events, considered finished when other emitters finish
};

/// Base class for particle emitters
///
/// Emitters control when and how many particles are spawned.
class W_PARTICLEPLUGIN_DLL WParticleEmitter : public WParticleModule
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEmitter, WParticleModule);

  friend class WParticleSystemInstance;
  friend class WParticleEmitterFactory;

protected:
  virtual bool IsContinuous() const;
  virtual void Process(WUInt64 uiNumElements) final override;

  /// Called once per update. Must return how many new particles to spawn.
  virtual WUInt32 ComputeSpawnCount(const WTime& tDiff) = 0;

  /// Called before ComputeSpawnCount(). Returns whether the emitter will spawn more particles.
  virtual WParticleEmitterState IsFinished() = 0;

  virtual void ProcessEventQueue(WParticleEventQueue queue);
};

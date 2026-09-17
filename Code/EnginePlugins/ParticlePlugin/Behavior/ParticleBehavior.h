#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WProcessingStream;
class WParticleSystemInstance;
class WParticleBehavior;

/// Base class for all particle behaviors
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory, WReflectedClass);

public:
  virtual const WRTTI* GetBehaviorType() const = 0;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const = 0;

  WParticleBehavior* CreateBehavior(WParticleSystemInstance* pOwner) const;

  virtual void Save(WStreamWriter& inout_stream) const = 0;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const {}
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior : public WParticleModule
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior, WParticleModule);

  friend class WParticleSystemInstance;

protected:
  WParticleBehavior();
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const WTime& tDiff, WUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  WTime m_TimeDiff;
};

#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WParticleSystemInstance;
class WProcessingStream;
class WParticleInitializer;
class WParticleEffectInstance;

/// Base class for all particle initializers
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory, WReflectedClass);

public:
  virtual const WRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const = 0;
  virtual float GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const;

  WParticleInitializer* CreateInitializer(WParticleSystemInstance* pOwner) const;

  virtual void Save(WStreamWriter& inout_stream) const = 0;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const {}
};

/// Base class for particle initializers
///
/// Initializers set the initial values of newly spawned particles.
/// They are executed once per particle when it is created.
class W_PARTICLEPLUGIN_DLL WParticleInitializer : public WParticleModule
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer, WParticleModule);

  friend class WParticleSystemInstance;
  friend class WParticleInitializerFactory;

protected:
  WParticleInitializer();

  virtual void Process(WUInt64 uiNumElements) final override {}
};

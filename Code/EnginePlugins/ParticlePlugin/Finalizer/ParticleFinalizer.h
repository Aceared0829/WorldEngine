#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WProcessingStream;
class WParticleSystemInstance;
class WParticleFinalizer;

/// Factory class for creating particle finalizer instances.
///
/// Finalizer factories hold the configuration properties and create the corresponding
/// finalizer instances when a particle system is instantiated.
class W_PARTICLEPLUGIN_DLL WParticleFinalizerFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizerFactory, WReflectedClass);

public:
  virtual const WRTTI* GetFinalizerType() const = 0;
  virtual void CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const = 0;

  WParticleFinalizer* CreateFinalizer(WParticleSystemInstance* pOwner) const;
};

/// Base class for particle finalizers.
///
/// Finalizers run at the end of each simulation frame to update particle state or
/// perform cleanup operations. They execute after all behaviors have been processed.
/// Examples include updating particle age, applying velocity to position, or computing
/// bounding volumes.
class W_PARTICLEPLUGIN_DLL WParticleFinalizer : public WParticleModule
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizer, WParticleModule);

  friend class WParticleSystemInstance;

protected:
  WParticleFinalizer();
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const WTime& tDiff, WUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  /// Time delta for the current simulation step.
  WTime m_TimeDiff;
};

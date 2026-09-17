#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class WPhysicsWorldModuleInterface;

/// Factory for volume finalizers.
class W_PARTICLEPLUGIN_DLL WParticleFinalizerFactory_Volume final : public WParticleFinalizerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizerFactory_Volume, WParticleFinalizerFactory);

public:
  WParticleFinalizerFactory_Volume();
  ~WParticleFinalizerFactory_Volume();

  virtual const WRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Computes the bounding volume for the entire particle system.
///
/// Calculates a bounding box-sphere from all particle positions and optionally considers
/// particle sizes to determine the maximum extent. The computed volume is used for culling
/// and rendering optimizations. If no particles exist, the process is skipped.
class W_PARTICLEPLUGIN_DLL WParticleFinalizer_Volume final : public WParticleFinalizer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizer_Volume, WParticleFinalizer);

public:
  WParticleFinalizer_Volume();
  ~WParticleFinalizer_Volume();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  const WProcessingStream* m_pStreamSize = nullptr;
};

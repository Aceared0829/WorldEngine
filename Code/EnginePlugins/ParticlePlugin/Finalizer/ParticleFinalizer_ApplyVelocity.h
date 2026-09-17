#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Factory for apply velocity finalizers.
class W_PARTICLEPLUGIN_DLL WParticleFinalizerFactory_ApplyVelocity final : public WParticleFinalizerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizerFactory_ApplyVelocity, WParticleFinalizerFactory);

public:
  WParticleFinalizerFactory_ApplyVelocity();

  virtual const WRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Integrates particle velocity into position each frame.
///
/// Updates particle positions by adding velocity * time_delta. The velocity is stored as
/// a direction vector (xyz) and speed scalar (w). This finalizer has a slightly higher
/// priority (525) to run after most other finalizers.
class W_PARTICLEPLUGIN_DLL WParticleFinalizer_ApplyVelocity final : public WParticleFinalizer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizer_ApplyVelocity, WParticleFinalizer);

public:
  WParticleFinalizer_ApplyVelocity();
  ~WParticleFinalizer_ApplyVelocity();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
};

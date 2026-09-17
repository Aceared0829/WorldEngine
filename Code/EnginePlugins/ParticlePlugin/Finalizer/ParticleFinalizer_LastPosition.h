#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Factory for last position finalizers.
class W_PARTICLEPLUGIN_DLL WParticleFinalizerFactory_LastPosition final : public WParticleFinalizerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizerFactory_LastPosition, WParticleFinalizerFactory);

public:
  WParticleFinalizerFactory_LastPosition();

  virtual const WRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Records the previous frame's particle position for motion-based effects.
///
/// Copies the current position to the last position stream each frame. This data is used
/// by renderers to create motion blur trails or stretched particles. The finalizer has a
/// very low priority (-499) to run early in the frame, after initializers but before
/// most other processing.
class W_PARTICLEPLUGIN_DLL WParticleFinalizer_LastPosition final : public WParticleFinalizer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizer_LastPosition, WParticleFinalizer);

public:
  WParticleFinalizer_LastPosition();
  ~WParticleFinalizer_LastPosition();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamLastPosition = nullptr;
};

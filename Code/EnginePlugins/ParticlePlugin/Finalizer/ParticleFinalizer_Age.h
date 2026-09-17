#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class WPhysicsWorldModuleInterface;

/// Factory for age finalizers.
class W_PARTICLEPLUGIN_DLL WParticleFinalizerFactory_Age final : public WParticleFinalizerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizerFactory_Age, WParticleFinalizerFactory);

public:
  WParticleFinalizerFactory_Age();

  virtual const WRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const override;

  WVarianceTypeTime m_LifeTime;
  WString m_sOnDeathEvent;
  WString m_sLifeScaleParameter;
};


/// Updates particle age and removes particles that have exceeded their lifetime.
///
/// Each frame, the remaining lifetime is decreased by the time delta. When a particle's
/// lifetime reaches zero, it is removed from the system. If an on-death event is configured,
/// the event is triggered at the particle's position with its velocity as the direction.
/// The lifetime is initialized with optional variance and can be scaled by an effect parameter.
class W_PARTICLEPLUGIN_DLL WParticleFinalizer_Age final : public WParticleFinalizer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleFinalizer_Age, WParticleFinalizer);

public:
  WParticleFinalizer_Age();
  ~WParticleFinalizer_Age();

  virtual void CreateRequiredStreams() override;

  WVarianceTypeTime m_LifeTime;
  WTempHashedString m_sOnDeathEvent;
  WTempHashedString m_sLifeScaleParameter;

protected:
  friend class WParticleFinalizerFactory_Age;

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override;
  void OnParticleDeath(const WStreamGroupElementRemovedEvent& e);

  bool m_bHasOnDeathEventHandler = false;
  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
};

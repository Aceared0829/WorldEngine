#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WParticleEmitterFactory;
class WParticleBehaviorFactory;
class WParticleInitializerFactory;
class WParticleTypeFactory;
class WParticleFinalizerFactory;

class W_PARTICLEPLUGIN_DLL WParticleSystemDescriptor final : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleSystemDescriptor, WReflectedClass);

public:
  WParticleSystemDescriptor();
  ~WParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const WHybridArray<WParticleEmitterFactory*, 1>& GetEmitterFactories() const { return m_EmitterFactories; }

  void AddInitializerFactory(WParticleInitializerFactory* pFactory) { m_InitializerFactories.PushBack(pFactory); }
  void RemoveInitializerFactory(WParticleInitializerFactory* pFactory) { m_InitializerFactories.RemoveAndCopy(pFactory); }
  const WHybridArray<WParticleInitializerFactory*, 4>& GetInitializerFactories() const { return m_InitializerFactories; }

  void AddBehaviorFactory(WParticleBehaviorFactory* pFactory) { m_BehaviorFactories.PushBack(pFactory); }
  void RemoveBehaviorFactory(WParticleBehaviorFactory* pFactory) { m_BehaviorFactories.RemoveAndCopy(pFactory); }
  const WHybridArray<WParticleBehaviorFactory*, 4>& GetBehaviorFactories() const { return m_BehaviorFactories; }

  void AddTypeFactory(WParticleTypeFactory* pFactory) { m_TypeFactories.PushBack(pFactory); }
  void RemoveTypeFactory(WParticleTypeFactory* pFactory) { m_TypeFactories.RemoveAndCopy(pFactory); }
  const WHybridArray<WParticleTypeFactory*, 2>& GetTypeFactories() const { return m_TypeFactories; }

  const WHybridArray<WParticleFinalizerFactory*, 2>& GetFinalizerFactories() const { return m_FinalizerFactories; }

  WTime GetAvgLifetime() const;

  bool m_bVisible;

  WVarianceTypeTime m_LifeTime;
  WString m_sOnDeathEvent;
  WString m_sLifeScaleParameter;

  //////////////////////////////////////////////////////////////////////////

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor);

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void ClearFinalizers();
  void SetupDefaultProcessors();

  WString m_sName;
  WHybridArray<WParticleEmitterFactory*, 1> m_EmitterFactories;
  WHybridArray<WParticleInitializerFactory*, 4> m_InitializerFactories;
  WHybridArray<WParticleBehaviorFactory*, 4> m_BehaviorFactories;
  WHybridArray<WParticleFinalizerFactory*, 2> m_FinalizerFactories;
  WHybridArray<WParticleTypeFactory*, 2> m_TypeFactories;
};

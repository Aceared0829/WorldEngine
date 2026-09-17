#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/IdTable.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

using WParticleEffectResourceHandle = WTypedResourceHandle<class WParticleEffectResource>;
class WParticleEffectInstance;
struct WResourceEvent;
class WTaskGroupID;
class WParticleStream;
class WParticleStreamFactory;

/// This world module stores all particle effect data that is active in a given WWorld instance
///
/// It is used to update all effects in one world and also to render them.
/// When an effect is stopped, it only stops emitting new particles, but it lives on until all particles are dead.
/// Therefore particle effects need to be managed outside of components. When a component dies, it only tells the
/// world module to 'destroy' it's effect, the rest is handled behind the scenes.
class W_PARTICLEPLUGIN_DLL WParticleWorldModule final : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WParticleWorldModule, WWorldModule);

public:
  WParticleWorldModule(WWorld* pWorld);
  ~WParticleWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  WParticleEffectHandle CreateEffectInstance(const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void*& inout_pSharedInstanceOwner, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams);

  /// This does not actually the effect, it first stops it from emitting and destroys it once all particles have actually died of old age.
  void DestroyEffectInstance(const WParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner);

  bool TryGetEffectInstance(const WParticleEffectHandle& hEffect, WParticleEffectInstance*& out_pEffect);
  bool TryGetEffectInstance(const WParticleEffectHandle& hEffect, const WParticleEffectInstance*& out_pEffect) const;

  /// Extracts render data for the given effect.
  void ExtractEffectRenderData(const WParticleEffectInstance* pEffect, WMsgExtractRenderData& ref_msg, const WTransform& systemTransform) const;

  WParticleSystemInstance* CreateSystemInstance(WUInt32 uiMaxParticles, WWorld* pWorld, WParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier);
  void DestroySystemInstance(WParticleSystemInstance* pInstance);

  WParticleStream* CreateStreamDefaultInitializer(WParticleSystemInstance* pOwner, const char* szFullStreamName) const;

  /// Can be called at any time (e.g. during WParticleBehaviorFactory::CopyBehaviorProperties()) to query a previously cached world module,
  /// even if that happens on a thread which would not be allowed to query this from the WWorld at that time.
  WWorldModule* GetCachedWorldModule(const WRTTI* pRtti) const;

  /// Should be called by WParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  template <class T>
  void CacheWorldModule()
  {
    CacheWorldModule(WGetStaticRTTI<T>());
  }

  /// Should be called by WParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  void CacheWorldModule(const WRTTI* pRtti);

private:
  virtual void WorldClear() override;

  void UpdateEffects(const WWorldModule::UpdateContext& context);
  void EnsureUpdatesFinished(const WWorldModule::UpdateContext& context);

  void DestroyFinishedEffects();
  void CreateFinisherComponent(WParticleEffectInstance* pEffect);
  void ResourceEventHandler(const WResourceEvent& e);
  void ReconfigureEffects();
  WParticleEffectHandle InternalCreateSharedEffectInstance(const char* szSharedName, const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, const void* pSharedInstanceOwner);
  WParticleEffectHandle InternalCreateEffectInstance(const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, bool bIsShared, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams);

  void ConfigureParticleStreamFactories();
  void ClearParticleStreamFactories();

  mutable WMutex m_Mutex;
  WDeque<WParticleEffectInstance> m_ParticleEffects;
  WDynamicArray<WParticleEffectInstance*> m_FinishingEffects;
  WDynamicArray<WParticleEffectInstance*> m_NeedFinisherComponent;
  WDynamicArray<WParticleEffectInstance*> m_EffectsToReconfigure;
  WDynamicArray<WParticleEffectInstance*> m_ParticleEffectsFreeList;
  WMap<WString, WParticleEffectHandle> m_SharedEffects;
  WIdTable<WParticleEffectId, WParticleEffectInstance*> m_ActiveEffects;
  WDeque<WParticleSystemInstance> m_ParticleSystems;
  WDynamicArray<WParticleSystemInstance*> m_ParticleSystemFreeList;
  WTaskGroupID m_EffectUpdateTaskGroup;
  WMap<WString, WParticleStreamFactory*> m_StreamFactories;
  WHashTable<const WRTTI*, WWorldModule*> m_WorldModuleCache;
};

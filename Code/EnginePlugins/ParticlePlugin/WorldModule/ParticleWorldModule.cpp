#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WParticleWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleWorldModule::WParticleWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

WParticleWorldModule::~WParticleWorldModule()
{
  ClearParticleStreamFactories();
}

void WParticleWorldModule::Initialize()
{
  ConfigureParticleStreamFactories();

  {
    auto updateDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WParticleWorldModule::UpdateEffects, this);
    updateDesc.m_Phase = WWorldUpdatePhase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;
    updateDesc.m_fPriority = 1000.0f; // kick off particle tasks as early as possible

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto finishDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WParticleWorldModule::EnsureUpdatesFinished, this);
    finishDesc.m_Phase = WWorldUpdatePhase::PostTransform;
    finishDesc.m_bOnlyUpdateWhenSimulating = true;
    finishDesc.m_fPriority = -1000.0f; // sync with particle tasks as late as possible

    RegisterUpdateFunction(finishDesc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WParticleWorldModule::ResourceEventHandler, this));

  WRTTI::ForEachDerivedType<WParticleModule>(
    [this](const WRTTI* pRtti)
    {
      WUniquePtr<WParticleModule> pModule = pRtti->GetAllocator()->Allocate<WParticleModule>();
      pModule->RequestRequiredWorldModulesForCache(this);
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);
}


void WParticleWorldModule::Deinitialize()
{
  W_LOCK(m_Mutex);

  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WParticleWorldModule::ResourceEventHandler, this));

  WorldClear();
}

void WParticleWorldModule::EnsureUpdatesFinished(const WWorldModule::UpdateContext& context)
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  WTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  {
    W_LOCK(m_Mutex);

    // The simulation tasks are done and the game objects have their global transform updated at this point, so we can push the transform
    // to the particle effects for the next simulation step and also ensure that the bounding volumes are correct for culling and rendering.
    if (WParticleComponentManager* pManager = GetWorld()->GetComponentManager<WParticleComponentManager>())
    {
      pManager->UpdatePfxTransformsAndBounds();
    }

    if (WParticleFinisherComponentManager* pManager = GetWorld()->GetComponentManager<WParticleFinisherComponentManager>())
    {
      pManager->UpdateBounds();
    }

    for (WUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
    {
      CreateFinisherComponent(m_NeedFinisherComponent[i]);
    }

    m_NeedFinisherComponent.Clear();
  }
}

void WParticleWorldModule::ExtractEffectRenderData(const WParticleEffectInstance* pEffect, WMsgExtractRenderData& ref_msg, const WTransform& systemTransform) const
{
  W_ASSERT_DEBUG(WTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  W_LOCK(m_Mutex);

  for (WUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
  {
    const WParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

    if (pSystem == nullptr)
      continue;

    if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
      continue;

    pSystem->ExtractSystemRenderData(ref_msg, systemTransform);
  }
}

void WParticleWorldModule::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WParticleEffectResource>())
  {
    W_LOCK(m_Mutex);

    WParticleEffectResourceHandle hResource((WParticleEffectResource*)(e.m_pResource));

    const WUInt32 numEffects = m_ParticleEffects.GetCount();
    for (WUInt32 i = 0; i < numEffects; ++i)
    {
      if (m_ParticleEffects[i].GetResource() == hResource)
      {
        m_EffectsToReconfigure.PushBack(&m_ParticleEffects[i]);
      }
    }
  }
}

void WParticleWorldModule::ConfigureParticleStreamFactories()
{
  ClearParticleStreamFactories();

  WRTTI::ForEachDerivedType<WParticleStreamFactory>(
    [&](const WRTTI* pRtti)
    {
      WParticleStreamFactory* pFactory = pRtti->GetAllocator()->Allocate<WParticleStreamFactory>();

      m_StreamFactories[pFactory->GetStreamName()] = pFactory;
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void WParticleWorldModule::ClearParticleStreamFactories()
{
  for (auto it : m_StreamFactories)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_StreamFactories.Clear();
}

WParticleStream* WParticleWorldModule::CreateStreamDefaultInitializer(WParticleSystemInstance* pOwner, const char* szFullStreamName) const
{
  auto it = m_StreamFactories.Find(szFullStreamName);
  if (!it.IsValid())
    return nullptr;

  return it.Value()->CreateParticleStream(pOwner);
}

WWorldModule* WParticleWorldModule::GetCachedWorldModule(const WRTTI* pRtti) const
{
  WWorldModule* pModule = nullptr;
  m_WorldModuleCache.TryGetValue(pRtti, pModule);
  return pModule;
}

void WParticleWorldModule::CacheWorldModule(const WRTTI* pRtti)
{
  m_WorldModuleCache[pRtti] = GetWorld()->GetOrCreateModule(pRtti);
}

void WParticleWorldModule::CreateFinisherComponent(WParticleEffectInstance* pEffect)
{
  if (pEffect && !pEffect->IsSharedEffect())
  {
    pEffect->SetVisibleIf(nullptr);

    WWorld* pWorld = GetWorld();

    const WTransform transform = pEffect->GetTransform();

    WGameObjectDesc go;
    go.m_LocalPosition = transform.m_vPosition;
    go.m_LocalRotation = transform.m_qRotation;
    go.m_LocalScaling = transform.m_vScale;
    // go.m_Tags = GetOwner()->GetTags(); // TODO: pass along tags -> needed for rendering filters

    WGameObject* pFinisher;
    pWorld->CreateObject(go, pFinisher);

    WParticleFinisherComponent* pFinisherComp;
    WParticleFinisherComponent::CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = WParticleEffectController(this, pEffect->GetHandle());
    pFinisherComp->m_EffectController.SetTransform(transform, WVec3::MakeZero()); // clear the velocity
  }
}

void WParticleWorldModule::WorldClear()
{
  // make sure no particle update task is still in the pipeline
  WTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  W_LOCK(m_Mutex);

  m_FinishingEffects.Clear();
  m_NeedFinisherComponent.Clear();

  m_ActiveEffects.Clear();
  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
  m_ParticleEffectsFreeList.Clear();
  m_ParticleSystemFreeList.Clear();
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleWorldModule);

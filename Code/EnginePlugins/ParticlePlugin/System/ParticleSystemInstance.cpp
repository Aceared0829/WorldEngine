#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

bool WParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

bool WParticleSystemInstance::IsEmitterConfigEqual(const WParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetEmitterFactories();

  if (factories.GetCount() != m_Emitters.GetCount())
    return false;

  for (WUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetEmitterType() != m_Emitters[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool WParticleSystemInstance::IsInitializerConfigEqual(const WParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetInitializerFactories();

  if (factories.GetCount() != m_Initializers.GetCount())
    return false;

  for (WUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetInitializerType() != m_Initializers[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool WParticleSystemInstance::IsBehaviorConfigEqual(const WParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetBehaviorFactories();

  if (factories.GetCount() != m_Behaviors.GetCount())
    return false;

  for (WUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetBehaviorType() != m_Behaviors[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool WParticleSystemInstance::IsTypeConfigEqual(const WParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetTypeFactories();

  if (factories.GetCount() != m_Types.GetCount())
    return false;

  for (WUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetTypeType() != m_Types[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool WParticleSystemInstance::IsFinalizerConfigEqual(const WParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetFinalizerFactories();

  if (factories.GetCount() != m_Finalizers.GetCount())
    return false;

  for (WUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetFinalizerType() != m_Finalizers[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

void WParticleSystemInstance::ConfigureFromTemplate(const WParticleSystemDescriptor* pTemplate)
{
  m_bVisible = pTemplate->m_bVisible;

  for (auto& info : m_StreamInfo)
  {
    info.m_bGetsInitialized = false;
    info.m_bInUse = false;
  }

  const bool allProcessorsEqual = IsEmitterConfigEqual(pTemplate) && IsInitializerConfigEqual(pTemplate) && IsBehaviorConfigEqual(pTemplate) && IsTypeConfigEqual(pTemplate) && IsFinalizerConfigEqual(pTemplate);

  if (!allProcessorsEqual)
  {
    // recreate emitters, initializers and behaviors
    CreateStreamProcessors(pTemplate);
  }
  else
  {
    // just re-initialize the emitters with the new properties
    ReinitializeStreamProcessors(pTemplate);
  }

  SetupOptionalStreams();

  // setup stream initializers for all streams that have none yet
  CreateStreamZeroInitializers();
}


void WParticleSystemInstance::Finalize()
{
  for (auto& pEmitter : m_Emitters)
  {
    pEmitter->OnFinalize();
  }

  for (auto& pInitializer : m_Initializers)
  {
    pInitializer->OnFinalize();
  }

  for (auto& pBehavior : m_Behaviors)
  {
    pBehavior->OnFinalize();
  }

  for (auto& pFinalizer : m_Finalizers)
  {
    pFinalizer->OnFinalize();
  }

  for (auto& pType : m_Types)
  {
    pType->OnFinalize();
  }
}

void WParticleSystemInstance::CreateStreamProcessors(const WParticleSystemDescriptor* pTemplate)
{
  // all spawners get cleared, so clear this as well
  // this has to be done before any streams get created
  // for (auto& info : m_StreamInfo)
  //{
  //  info.m_pZeroInitializer = nullptr;
  //}


  const WUInt64 uiMaxParticles = m_StreamGroup.GetNumElements();
  m_StreamGroup.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
  m_StreamInfo.Clear();

  // emitters
  {
    m_Emitters.Clear();

    for (const auto pFactory : pTemplate->GetEmitterFactories())
    {
      WParticleEmitter* pEmitter = pFactory->CreateEmitter(this);
      m_StreamGroup.AddProcessor(pEmitter);
      m_Emitters.PushBack(pEmitter);
    }
  }

  // initializers
  {
    m_Initializers.Clear();

    for (const auto pFactory : pTemplate->GetInitializerFactories())
    {
      WParticleInitializer* pInitializer = pFactory->CreateInitializer(this);
      m_StreamGroup.AddProcessor(pInitializer);
      m_Initializers.PushBack(pInitializer);
    }
  }

  // behaviors
  {
    m_Behaviors.Clear();

    for (const auto pFactory : pTemplate->GetBehaviorFactories())
    {
      WParticleBehavior* pBehavior = pFactory->CreateBehavior(this);
      m_StreamGroup.AddProcessor(pBehavior);
      m_Behaviors.PushBack(pBehavior);
    }
  }

  // finalizers
  {
    m_Finalizers.Clear();

    for (const auto pFactory : pTemplate->GetFinalizerFactories())
    {
      WParticleFinalizer* pFinalizer = pFactory->CreateFinalizer(this);
      m_StreamGroup.AddProcessor(pFinalizer);
      m_Finalizers.PushBack(pFinalizer);
    }
  }

  // types
  {
    m_Types.Clear();

    for (const auto pFactory : pTemplate->GetTypeFactories())
    {
      WParticleType* pType = pFactory->CreateType(this);
      m_StreamGroup.AddProcessor(pType);
      m_Types.PushBack(pType);
    }
  }
}


void WParticleSystemInstance::SetupOptionalStreams()
{
  for (auto& pEmitter : m_Emitters)
  {
    pEmitter->QueryOptionalStreams();
  }

  for (auto& pInitializer : m_Initializers)
  {
    pInitializer->QueryOptionalStreams();
  }

  for (auto& pBehavior : m_Behaviors)
  {
    pBehavior->QueryOptionalStreams();
  }

  for (auto& pFinalizer : m_Finalizers)
  {
    pFinalizer->QueryOptionalStreams();
  }

  for (auto& pType : m_Types)
  {
    pType->QueryOptionalStreams();
  }
}

void WParticleSystemInstance::SetTransform(const WTransform& transform, const WVec3& vParticleStartVelocity)
{
  m_Transform = transform;
  m_vParticleStartVelocity = vParticleStartVelocity;
}

void WParticleSystemInstance::ReinitializeStreamProcessors(const WParticleSystemDescriptor* pTemplate)
{
  // emitters
  {
    const auto& factories = pTemplate->GetEmitterFactories();

    for (WUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Emitters[i]->Reset(this);
      factories[i]->CopyEmitterProperties(m_Emitters[i], false);
      m_Emitters[i]->CreateRequiredStreams();
    }
  }

  // initializers
  {
    const auto& factories = pTemplate->GetInitializerFactories();

    for (WUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Initializers[i]->Reset(this);
      factories[i]->CopyInitializerProperties(m_Initializers[i], false);
      m_Initializers[i]->CreateRequiredStreams();
    }
  }

  // behaviors
  {
    const auto& factories = pTemplate->GetBehaviorFactories();

    for (WUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Behaviors[i]->Reset(this);
      factories[i]->CopyBehaviorProperties(m_Behaviors[i], false);
      m_Behaviors[i]->CreateRequiredStreams();
    }
  }

  // finalizers
  {
    const auto& factories = pTemplate->GetFinalizerFactories();

    for (WUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Finalizers[i]->Reset(this);
      factories[i]->CopyFinalizerProperties(m_Finalizers[i], false);
      m_Finalizers[i]->CreateRequiredStreams();
    }
  }

  // types
  {
    const auto& factories = pTemplate->GetTypeFactories();

    for (WUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Types[i]->Reset(this);
      factories[i]->CopyTypeProperties(m_Types[i], false);
      m_Types[i]->CreateRequiredStreams();
    }
  }
}

WParticleSystemInstance::WParticleSystemInstance()
{
  m_BoundingVolume = WBoundingBoxSphere::MakeInvalid();
}

void WParticleSystemInstance::Construct(WUInt32 uiMaxParticles, WWorld* pWorld, WParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier)
{
  m_BoundingVolume = WBoundingBoxSphere::MakeInvalid();
  m_Transform.SetIdentity();
  m_pOwnerEffect = pOwnerEffect;
  m_bEmitterEnabled = true;
  m_bVisible = true;
  m_pWorld = pWorld;
  m_fSpawnCountMultiplier = fSpawnCountMultiplier;

  m_StreamInfo.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
}

void WParticleSystemInstance::Destruct()
{
  m_StreamGroup.Clear();
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();
  m_Finalizers.Clear();
  m_Types.Clear();

  m_StreamInfo.Clear();
}

WParticleSystemState::Enum WParticleSystemInstance::Update(const WTime& diff)
{
  W_PROFILE_SCOPE("PFX: System Update");

  WUInt32 uiSpawnedParticles = 0;

  if (m_bEmitterEnabled)
  {
    // if all emitters are finished, we deactivate this on the whole system
    bool bAllEmittersInactive = true;

    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == WParticleEmitterState::Active)
      {
        bAllEmittersInactive = false;
        const WUInt32 uiSpawn = pEmitter->ComputeSpawnCount(diff);

        if (uiSpawn > 0)
        {
          W_PROFILE_SCOPE("PFX: System Emit");
          m_StreamGroup.InitializeElements(uiSpawn);
          uiSpawnedParticles += uiSpawn;
        }
      }
    }

    if (bAllEmittersInactive)
    {
      // there is a race condition writing this variable when an effect is used by a WParticleTypeEffect and should be disabled
      // therefore we must never set this variable to 'true' here, but we can set it to 'false' once we are done
      m_bEmitterEnabled = false;
    }
  }

  bool bHasReactingEmitters = false;

  // always check reactive emitters, as long as there are particles alive, they might produce more
  {
    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == WParticleEmitterState::OnlyReacting)
      {
        bHasReactingEmitters = true;

        const WUInt32 uiSpawn = pEmitter->ComputeSpawnCount(diff);

        if (uiSpawn > 0)
        {
          W_PROFILE_SCOPE("PFX: System Emit (React)");
          m_StreamGroup.InitializeElements(uiSpawn);
          uiSpawnedParticles += uiSpawn;
        }
      }
    }
  }

  {
    W_PROFILE_SCOPE("PFX: System Step Behaviors");
    for (auto pBehavior : m_Behaviors)
    {
      pBehavior->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    W_PROFILE_SCOPE("PFX: System Step Finalizers");
    for (auto pFinalizer : m_Finalizers)
    {
      pFinalizer->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    W_PROFILE_SCOPE("PFX: System Step Types");
    for (auto pType : m_Types)
    {
      pType->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    W_PROFILE_SCOPE("PFX: System Process");
    m_StreamGroup.Process();
  }

  if (m_bEmitterEnabled)
    return WParticleSystemState::Active;

  // all emitters are done, but some particles are still alive
  if (HasActiveParticles())
    return WParticleSystemState::EmittersFinished;

  return bHasReactingEmitters ? WParticleSystemState::OnlyReacting : WParticleSystemState::Inactive;
}

WProcessingStream* WParticleSystemInstance::QueryStream(WTempHashedString sName, WProcessingStream::DataType type) const
{
  return m_StreamGroup.GetStreamByName(sName);
}

void WParticleSystemInstance::CreateStream(WStringView sName, WProcessingStream::DataType type, WProcessingStream** pStream, WParticleStreamBinding& inout_binding, bool bWillInitializeElements)
{
  W_ASSERT_DEV(pStream != nullptr, "The pointer to the stream pointer must not be null");

  StreamInfo* pInfo = nullptr;

  WHashedString sNameHashed;
  sNameHashed.Assign(sName);

  WProcessingStream* pSubStream = m_StreamGroup.GetStreamByName(sNameHashed);
  if (pSubStream == nullptr)
  {
    pSubStream = m_StreamGroup.AddStream(sName, type);

    pInfo = &m_StreamInfo.ExpandAndGetRef();
    pInfo->m_sName = sNameHashed;
  }
  else
  {
    W_ASSERT_DEV(pSubStream->GetDataType() == type, "Particle stream '{}' already exists with data type {} (!= {}).", sName, (int)pSubStream->GetDataType(), (int)type);

    for (auto& info : m_StreamInfo)
    {
      if (info.m_sName == sNameHashed)
      {
        pInfo = &info;
        break;
      }
    }

    W_ASSERT_DEV(pInfo != nullptr, "Could not find info for stream");
  }

  pInfo->m_bInUse = true;
  if (bWillInitializeElements)
    pInfo->m_bGetsInitialized = true;

  W_ASSERT_DEV(pSubStream != nullptr, "Stream creation failed ('{0}')", sName);
  *pStream = pSubStream;

  {
    auto& bind = inout_binding.m_Bindings.ExpandAndGetRef();
    bind.m_ppStream = pStream;
    bind.m_sName = sNameHashed;
  }
}

void WParticleSystemInstance::CreateStreamZeroInitializers()
{
  for (WUInt32 i = 0; i < m_StreamInfo.GetCount();)
  {
    auto& info = m_StreamInfo[i];

    if ((!info.m_bInUse || info.m_bGetsInitialized) && info.m_pDefaultInitializer)
    {
      m_StreamGroup.RemoveProcessor(info.m_pDefaultInitializer);
      info.m_pDefaultInitializer = nullptr;
    }

    if (!info.m_bInUse)
    {
      m_StreamGroup.RemoveStreamByName(info.m_sName);
      m_StreamInfo.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (auto& info : m_StreamInfo)
  {
    if (info.m_bGetsInitialized)
      continue;

    W_ASSERT_DEV(info.m_bInUse, "Invalid state");

    if (info.m_pDefaultInitializer == nullptr)
    {
      WParticleStream* pStream = GetOwnerWorldModule()->CreateStreamDefaultInitializer(this, info.m_sName);

      if (pStream == nullptr)
      {
        WLog::Warning("Particle stream '{0}' is zero-initialized.", info.m_sName);

        WProcessingStreamSpawnerZeroInitialized* pZeroInit = W_DEFAULT_NEW(WProcessingStreamSpawnerZeroInitialized);
        pZeroInit->SetStreamName(info.m_sName);

        info.m_pDefaultInitializer = pZeroInit;
      }
      else
      {
        // WLog::Debug("Particle stream '{0}' is default-initialized.", info.m_sName);
        info.m_pDefaultInitializer = pStream;
      }

      m_StreamGroup.AddProcessor(info.m_pDefaultInitializer);
    }
  }
}

void WParticleStreamBinding::UpdateBindings(const WProcessingStreamGroup* pGroup) const
{
  for (const auto& bind : m_Bindings)
  {
    WProcessingStream* pStream = pGroup->GetStreamByName(bind.m_sName);
    W_ASSERT_DEV(pStream != nullptr, "Stream binding '{0}' is invalid now", bind.m_sName);

    *bind.m_ppStream = pStream;
  }
}

void WParticleSystemInstance::ProcessEventQueue(WParticleEventQueue queue)
{
  for (auto pEmitter : m_Emitters)
  {
    pEmitter->ProcessEventQueue(queue);
  }
}

WParticleWorldModule* WParticleSystemInstance::GetOwnerWorldModule() const
{
  return m_pOwnerEffect->GetOwnerWorldModule();
}

void WParticleSystemInstance::ExtractSystemRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  for (auto pType : m_Types)
  {
    pType->ExtractTypeRenderData(ref_msg, instanceTransform);
  }
}

void WParticleSystemInstance::AddParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.AddEventHandler(handler);
}

void WParticleSystemInstance::RemoveParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.RemoveEventHandler(handler);
}

void WParticleSystemInstance::SetBoundingVolume(const WBoundingBoxSphere& volume, float fMaxParticleSize)
{
  m_BoundingVolume = volume;

  float fExpand = 0;
  for (const auto pType : m_Types)
  {
    fExpand = WMath::Max(fExpand, pType->GetMaxParticleRadius(fMaxParticleSize));
  }

  m_BoundingVolume.m_vBoxHalfExtents += WVec3(fExpand);
  m_BoundingVolume.m_fSphereRadius += fExpand;
}

bool WParticleSystemInstance::IsContinuous() const
{
  for (const WParticleEmitter* pEmitter : m_Emitters)
  {
    if (pEmitter->IsContinuous())
      return true;
  }

  return false;
}

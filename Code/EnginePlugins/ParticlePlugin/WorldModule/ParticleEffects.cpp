#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WParticleEffectHandle WParticleWorldModule::InternalCreateEffectInstance(const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, bool bIsShared, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams)
{
  W_LOCK(m_Mutex);

  WParticleEffectInstance* pInstance = nullptr;

  if (!m_ParticleEffectsFreeList.IsEmpty())
  {
    pInstance = m_ParticleEffectsFreeList.PeekBack();
    m_ParticleEffectsFreeList.PopBack();
  }
  else
  {
    pInstance = &m_ParticleEffects.ExpandAndGetRef();
  }

  WParticleEffectHandle hEffectHandle(m_ActiveEffects.Insert(pInstance));
  pInstance->Construct(hEffectHandle, hResource, GetWorld(), this, uiRandomSeed, bIsShared, floatParams, colorParams);

  return hEffectHandle;
}

WParticleEffectHandle WParticleWorldModule::InternalCreateSharedEffectInstance(
  const char* szSharedName, const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, const void* pSharedInstanceOwner)
{
  W_LOCK(m_Mutex);

  WStringBuilder fullName;
  fullName.SetFormat("{{0}}-{{1}}[{2}]", szSharedName, hResource.GetResourceID(), uiRandomSeed);

  bool bExisted = false;
  auto it = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  WParticleEffectInstance* pEffect = nullptr;

  if (bExisted)
  {
    TryGetEffectInstance(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() =
      InternalCreateEffectInstance(hResource, uiRandomSeed, true, WArrayPtr<WParticleEffectFloatParam>(), WArrayPtr<WParticleEffectColorParam>());
    TryGetEffectInstance(it.Value(), pEffect);
  }

  W_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}


WParticleEffectHandle WParticleWorldModule::CreateEffectInstance(const WParticleEffectResourceHandle& hResource, WUInt64 uiRandomSeed, const char* szSharedName, const void*& inout_pSharedInstanceOwner, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams)
{
  W_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  bool bIsShared = !WStringUtils::IsNullOrEmpty(szSharedName) && (inout_pSharedInstanceOwner != nullptr);

  if (!bIsShared)
  {
    WResourceLock<WParticleEffectResource> pResource(hResource, WResourceAcquireMode::BlockTillLoaded);
    bIsShared |= pResource->GetDescriptor().m_Effect.m_bAlwaysShared;
  }

  if (!bIsShared)
  {
    inout_pSharedInstanceOwner = nullptr;
    return InternalCreateEffectInstance(hResource, uiRandomSeed, false, floatParams, colorParams);
  }
  else
  {
    return InternalCreateSharedEffectInstance(szSharedName, hResource, uiRandomSeed, inout_pSharedInstanceOwner);
  }
}

void WParticleWorldModule::DestroyEffectInstance(const WParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner)
{
  W_LOCK(m_Mutex);

  WParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffectInstance(hEffect, pInstance))
  {
    if (pSharedInstanceOwner != nullptr)
    {
      pInstance->RemoveSharedInstance(pSharedInstanceOwner);
      return; // never delete these
    }

    if (!pInstance->m_bIsFinishing)
    {
      // make sure not to insert it into m_FinishingEffects twice
      pInstance->m_bIsFinishing = true;
      pInstance->SetEmitterEnabled(false);
      m_FinishingEffects.PushBack(pInstance);

      if (!bInterruptImmediately)
      {
        m_NeedFinisherComponent.PushBack(pInstance);
      }
    }

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}

bool WParticleWorldModule::TryGetEffectInstance(const WParticleEffectHandle& hEffect, WParticleEffectInstance*& out_pEffect)
{
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}

bool WParticleWorldModule::TryGetEffectInstance(const WParticleEffectHandle& hEffect, const WParticleEffectInstance*& out_pEffect) const
{
  WParticleEffectInstance* pEffect = nullptr;
  bool bResult = m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), pEffect);
  out_pEffect = pEffect;
  return bResult;
}

void WParticleWorldModule::UpdateEffects(const WWorldModule::UpdateContext& context)
{
  W_LOCK(m_Mutex);

  DestroyFinishedEffects();
  ReconfigureEffects();

  m_EffectUpdateTaskGroup = WTaskSystem::CreateTaskGroup(WTaskPriority::LateThisFrame);

  const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (WUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (!m_ParticleEffects[i].ShouldBeUpdated())
      continue;

    m_ParticleEffects[i].ProcessEventQueues();

    const WSharedPtr<WTask>& pTask = m_ParticleEffects[i].GetUpdateTask();
    static_cast<WParticleEffectUpdateTask*>(pTask.Borrow())->m_UpdateDiff = tDiff;

    WTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  WTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void WParticleWorldModule::DestroyFinishedEffects()
{
  W_LOCK(m_Mutex);

  for (WUInt32 i = 0; i < m_FinishingEffects.GetCount();)
  {
    WParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      if (m_ActiveEffects.Remove(pEffect->GetHandle().GetInternalID()))
      {
        pEffect->Destruct();

        m_ParticleEffectsFreeList.PushBack(pEffect);
      }

      m_FinishingEffects.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (WUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
  {
    WParticleEffectInstance* pEffect = m_NeedFinisherComponent[i];

    CreateFinisherComponent(pEffect);
  }

  m_NeedFinisherComponent.Clear();
}

void WParticleWorldModule::ReconfigureEffects()
{
  W_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(false, WArrayPtr<WParticleEffectFloatParam>(), WArrayPtr<WParticleEffectColorParam>());
  }

  m_EffectsToReconfigure.Clear();
}

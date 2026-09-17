#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

WParticleEffectController::WParticleEffectController()
{
  m_hEffect.Invalidate();
}

WParticleEffectController::WParticleEffectController(const WParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

WParticleEffectController::WParticleEffectController(WParticleWorldModule* pModule, WParticleEffectHandle hEffect)
{
  m_pModule = pModule;
  m_hEffect = hEffect;
}

void WParticleEffectController::operator=(const WParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

WParticleEffectInstance* WParticleEffectController::GetInstance() const
{
  if (m_pModule == nullptr)
    return nullptr;

  WParticleEffectInstance* pEffect = nullptr;
  m_pModule->TryGetEffectInstance(m_hEffect, pEffect);
  return pEffect;
}

void WParticleEffectController::Create(const WParticleEffectResourceHandle& hEffectResource, WParticleWorldModule* pModule, WUInt64 uiRandomSeed,
  const char* szSharedName, const void* pSharedInstanceOwner, WArrayPtr<WParticleEffectFloatParam> floatParams,
  WArrayPtr<WParticleEffectColorParam> colorParams)
{
  m_pSharedInstanceOwner = pSharedInstanceOwner;

  // first get the new effect, to potentially increase a refcount to the same effect instance, before we decrease the refcount of our
  // current one
  WParticleEffectHandle hNewEffect;
  if (pModule != nullptr && hEffectResource.IsValid())
  {
    hNewEffect = pModule->CreateEffectInstance(hEffectResource, uiRandomSeed, szSharedName, m_pSharedInstanceOwner, floatParams, colorParams);
  }

  Invalidate();

  m_hEffect = hNewEffect;

  if (!m_hEffect.IsInvalidated())
    m_pModule = pModule;
}

bool WParticleEffectController::IsValid() const
{
  return (m_pModule != nullptr && !m_hEffect.IsInvalidated());
}

bool WParticleEffectController::IsAlive() const
{
  WParticleEffectInstance* pEffect = GetInstance();
  return pEffect != nullptr;
}

void WParticleEffectController::SetTransform(const WTransform& t, const WVec3& vParticleStartVelocity) const
{
  WParticleEffectInstance* pEffect = GetInstance();

  // shared effects are always simulated at the origin
  if (pEffect && m_pSharedInstanceOwner == nullptr)
  {
    pEffect->SetTransform(t, vParticleStartVelocity);
  }
}

void WParticleEffectController::CombineSystemBoundingVolumes()
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->CombineSystemBoundingVolumes();
  }
}

void WParticleEffectController::Tick(const WTime& diff) const
{
  WParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->PreSimulate();
    pEffect->Update(diff);
  }
}

void WParticleEffectController::ExtractRenderData(WMsgExtractRenderData& ref_msg, const WTransform& systemTransform) const
{
  if (const WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();

    m_pModule->ExtractEffectRenderData(pEffect, ref_msg, systemTransform);
  }
}

void WParticleEffectController::StopImmediate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, true, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

void WParticleEffectController::GetBoundingVolume(WBoundingBoxSphere& ref_volume) const
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->GetBoundingVolume(ref_volume);
  }
}

void WParticleEffectController::UpdateWindSamples(WTime diff)
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->UpdateWindSamples(diff);
  }
}

void WParticleEffectController::FindNearbyAttractors(WTime diff)
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->FindNearbyAttractors(diff);
  }
}

void WParticleEffectController::ForceVisible()
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();
  }
}

WUInt64 WParticleEffectController::GetNumActiveParticles() const
{
  if (WParticleEffectInstance* pEffect = GetInstance())
  {
    return pEffect->GetNumActiveParticles();
  }

  return 0;
}

void WParticleEffectController::SetParameter(const WTempHashedString& sName, float value)
{
  WParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(sName, value);
  }
}

void WParticleEffectController::SetParameter(const WTempHashedString& sName, const WColor& value)
{
  WParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(sName, value);
  }
}

void WParticleEffectController::Invalidate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, false, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

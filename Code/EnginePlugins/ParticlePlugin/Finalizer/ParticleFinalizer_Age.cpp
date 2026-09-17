#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizerFactory_Age, 1, WRTTIDefaultAllocator<WParticleFinalizerFactory_Age>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizer_Age, 1, WRTTIDefaultAllocator<WParticleFinalizer_Age>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleFinalizerFactory_Age::WParticleFinalizerFactory_Age() = default;

const WRTTI* WParticleFinalizerFactory_Age::GetFinalizerType() const
{
  return WGetStaticRTTI<WParticleFinalizer_Age>();
}

void WParticleFinalizerFactory_Age::CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const
{
  WParticleFinalizer_Age* pFinalizer = static_cast<WParticleFinalizer_Age*>(pObject);

  pFinalizer->m_LifeTime = m_LifeTime;
  pFinalizer->m_sOnDeathEvent = WTempHashedString(m_sOnDeathEvent.GetData());
  pFinalizer->m_sLifeScaleParameter = WTempHashedString(m_sLifeScaleParameter.GetData());

  if (pFinalizer->m_bHasOnDeathEventHandler)
  {
    pFinalizer->m_bHasOnDeathEventHandler = false;
    pFinalizer->GetOwnerSystem()->RemoveParticleDeathEventHandler(WMakeDelegate(&WParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }

  if (!pFinalizer->m_sOnDeathEvent.IsEmpty())
  {
    pFinalizer->m_bHasOnDeathEventHandler = true;
    pFinalizer->GetOwnerSystem()->AddParticleDeathEventHandler(WMakeDelegate(&WParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }
}

WParticleFinalizer_Age::WParticleFinalizer_Age() = default;

WParticleFinalizer_Age::~WParticleFinalizer_Age()
{
  if (m_bHasOnDeathEventHandler)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(WMakeDelegate(&WParticleFinalizer_Age::OnParticleDeath, this));
  }
}

void WParticleFinalizer_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, true);

  m_pStreamPosition = nullptr;
  m_pStreamVelocity = nullptr;

  if (!m_sOnDeathEvent.IsEmpty())
  {
    CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }
}

void WParticleFinalizer_Age::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Age Init");

  WFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<WFloat16Vec2>();
  const float fLifeScale = WMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f, 2.0f);

  if (m_LifeTime.m_fVariance == 0)
  {
    const float tLifeTime = WMath::Max(fLifeScale * (float)m_LifeTime.m_Value.GetSeconds(), 0.01f); // make sure it's not zero
    const float tInvLifeTime = 1.0f / tLifeTime;

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
  else // random range
  {
    WRandom& rng = GetRNG();

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime =
        WMath::Max(fLifeScale * (float)rng.DoubleVariance(m_LifeTime.m_Value.GetSeconds(), m_LifeTime.m_fVariance), 0.01f); // make sure it's not zero
      const float tInvLifeTime = 1.0f / tLifeTime;

      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
}

void WParticleFinalizer_Age::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Age");

  WFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<WFloat16Vec2>();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  for (WUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x = pLifeTime[i].x - tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      m_pStreamGroup->RemoveElement(i);
    }
  }
}

void WParticleFinalizer_Age::OnParticleDeath(const WStreamGroupElementRemovedEvent& e)
{
  const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
  const WFloat16Vec4* pVelocity = m_pStreamVelocity->GetData<WFloat16Vec4>();

  const WVec4 vel = pVelocity[e.m_uiElementIndex];
  const WVec3 dir(vel.x, vel.y, vel.z);
  const float speed = vel.w;

  WParticleEvent pe;
  pe.m_EventType = m_sOnDeathEvent;
  pe.m_vPosition = pPosition[e.m_uiElementIndex].GetAsVec3();
  pe.m_vDirection = dir * speed;
  pe.m_vNormal.SetZero();

  GetOwnerEffect()->AddParticleEvent(pe);
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Age);

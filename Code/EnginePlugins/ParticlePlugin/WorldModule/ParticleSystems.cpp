#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

WParticleSystemInstance* WParticleWorldModule::CreateSystemInstance(
  WUInt32 uiMaxParticles, WWorld* pWorld, WParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier)
{
  W_LOCK(m_Mutex);

  WParticleSystemInstance* pResult = nullptr;

  if (!m_ParticleSystemFreeList.IsEmpty())
  {
    pResult = m_ParticleSystemFreeList.PeekBack();
    m_ParticleSystemFreeList.PopBack();
  }

  if (pResult == nullptr)
  {
    pResult = &m_ParticleSystems.ExpandAndGetRef();
  }

  pResult->Construct(uiMaxParticles, pWorld, pOwnerEffect, fSpawnMultiplier);

  return pResult;
}

void WParticleWorldModule::DestroySystemInstance(WParticleSystemInstance* pInstance)
{
  W_LOCK(m_Mutex);

  W_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  pInstance->Destruct();
  m_ParticleSystemFreeList.PushBack(pInstance);
}

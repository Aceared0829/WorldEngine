#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;


WParticleBehavior* WParticleBehaviorFactory::CreateBehavior(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetBehaviorType();

  WParticleBehavior* pBehavior = pRtti->GetAllocator()->Allocate<WParticleBehavior>();
  pBehavior->Reset(pOwner);

  CopyBehaviorProperties(pBehavior, true);
  pBehavior->CreateRequiredStreams();

  return pBehavior;
}

WParticleBehavior::WParticleBehavior()
{
  // run after the initializers, before the types
  m_fPriority = 0.0f;
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior);

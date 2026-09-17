#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleInitializer* WParticleInitializerFactory::CreateInitializer(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetInitializerType();

  WParticleInitializer* pInitializer = pRtti->GetAllocator()->Allocate<WParticleInitializer>();
  pInitializer->Reset(pOwner);

  CopyInitializerProperties(pInitializer, true);
  pInitializer->CreateRequiredStreams();

  return pInitializer;
}

float WParticleInitializerFactory::GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const
{
  return 1.0f;
}

WParticleInitializer::WParticleInitializer()
{
  // run these early, but after the stream default initializers
  m_fPriority = -500.0f;
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer);

#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizerFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizer, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleFinalizer* WParticleFinalizerFactory::CreateFinalizer(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetFinalizerType();

  WParticleFinalizer* pFinalizer = pRtti->GetAllocator()->Allocate<WParticleFinalizer>();
  pFinalizer->Reset(pOwner);

  CopyFinalizerProperties(pFinalizer, true);
  pFinalizer->CreateRequiredStreams();

  return pFinalizer;
}

WParticleFinalizer::WParticleFinalizer()
{
  // run after the behaviors, before the types
  m_fPriority = +500.0f;
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer);

#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitterFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitter, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleEmitter* WParticleEmitterFactory::CreateEmitter(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetEmitterType();

  WParticleEmitter* pEmitter = pRtti->GetAllocator()->Allocate<WParticleEmitter>();
  pEmitter->Reset(pOwner);

  CopyEmitterProperties(pEmitter, true);
  pEmitter->CreateRequiredStreams();

  return pEmitter;
}

bool WParticleEmitter::IsContinuous() const
{
  return false;
}

void WParticleEmitter::Process(WUInt64 uiNumElements) {}
void WParticleEmitter::ProcessEventQueue(WParticleEventQueue queue) {}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter);

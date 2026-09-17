#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizerFactory_ApplyVelocity, 1, WRTTIDefaultAllocator<WParticleFinalizerFactory_ApplyVelocity>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizer_ApplyVelocity, 1, WRTTIDefaultAllocator<WParticleFinalizer_ApplyVelocity>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleFinalizerFactory_ApplyVelocity::WParticleFinalizerFactory_ApplyVelocity() = default;

const WRTTI* WParticleFinalizerFactory_ApplyVelocity::GetFinalizerType() const
{
  return WGetStaticRTTI<WParticleFinalizer_ApplyVelocity>();
}

void WParticleFinalizerFactory_ApplyVelocity::CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const
{
  WParticleFinalizer_ApplyVelocity* pFinalizer = static_cast<WParticleFinalizer_ApplyVelocity*>(pObject);
}

WParticleFinalizer_ApplyVelocity::WParticleFinalizer_ApplyVelocity()
{
  // a bit later than the other finalizers
  m_fPriority = 525.0f;
}

WParticleFinalizer_ApplyVelocity::~WParticleFinalizer_ApplyVelocity() = default;

void WParticleFinalizer_ApplyVelocity::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
}

void WParticleFinalizer_ApplyVelocity::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: ApplyVelocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  WProcessingStreamIterator<WVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    WVec3& pos = reinterpret_cast<WVec3&>(itPosition.Current());

    const WVec4 vel = itVelocity.Current();
    const WVec3 dir(vel.x, vel.y, vel.z);
    const float speed = vel.w;

    pos += dir * speed * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);

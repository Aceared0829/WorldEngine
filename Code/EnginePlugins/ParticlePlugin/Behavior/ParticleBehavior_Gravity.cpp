#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Gravity, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Gravity>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Gravity, 1, WRTTIDefaultAllocator<WParticleBehavior_Gravity>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Gravity::WParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}

const WRTTI* WParticleBehaviorFactory_Gravity::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Gravity>();
}

void WParticleBehaviorFactory_Gravity::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Gravity* pBehavior = static_cast<WParticleBehavior_Gravity*>(pObject);

  pBehavior->m_fGravityFactor = m_fGravityFactor;

  pBehavior->m_pPhysicsModule = (WPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(WGetStaticRTTI<WPhysicsWorldModuleInterface>());
}

void WParticleBehaviorFactory_Gravity::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fGravityFactor;
}

void WParticleBehaviorFactory_Gravity::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fGravityFactor;
}

void WParticleBehaviorFactory_Gravity::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void WParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
}

void WParticleBehavior_Gravity::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Gravity");

  const WVec3 vGravity = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity() : WVec3(0.0f, 0.0f, -10.0f);

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const WVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itVelocity.HasReachedEnd())
  {
    const WVec4 vel = itVelocity.Current();
    const WVec3 dir(vel.x, vel.y, vel.z);
    const float speed = vel.w;

    const WVec3 newVel = dir * speed + addGravity;
    const float newSpeed = newVel.GetLength();
    const WVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : WVec3(0, 0, 1);

    itVelocity.Current() = WVec4(newDir.x, newDir.y, newDir.z, newSpeed);

    itVelocity.Advance();
  }
}

void WParticleBehavior_Gravity::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WPhysicsWorldModuleInterface>();
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Gravity);

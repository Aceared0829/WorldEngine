#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Wind.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Wind, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Wind>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f), new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Wind, 1, WRTTIDefaultAllocator<WParticleBehavior_Wind>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Wind::WParticleBehaviorFactory_Wind() = default;
WParticleBehaviorFactory_Wind::~WParticleBehaviorFactory_Wind() = default;

const WRTTI* WParticleBehaviorFactory_Wind::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Wind>();
}

void WParticleBehaviorFactory_Wind::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Wind* pBehavior = static_cast<WParticleBehavior_Wind*>(pObject);

  pBehavior->m_fWindInfluence = m_fWindInfluence;
}

enum class BehaviorWindVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_Wind::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorWindVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fWindInfluence;
}

void WParticleBehaviorFactory_Wind::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorWindVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fWindInfluence;
}

void WParticleBehavior_Wind::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_fWindInfluence > 0)
  {
    GetOwnerEffect()->RequestWindSamples();
  }
}

void WParticleBehavior_Wind::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Wind");

  if (m_fWindInfluence <= 0)
    return;

  auto pOwner = GetOwnerEffect();
  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const WSimdFloat fWindFactor = m_fWindInfluence * tDiff;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    WSimdVec4f windOffset = pOwner->GetWindAt(itPosition.Current()) * fWindFactor;
    itPosition.Current() += windOffset;

    itPosition.Advance();
  }
}

void WParticleBehavior_Wind::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WWindWorldModuleInterface>();
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Wind);

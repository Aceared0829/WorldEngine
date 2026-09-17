#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_PullAlong.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_PullAlong, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_PullAlong>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_PullAlong, 1, WRTTIDefaultAllocator<WParticleBehavior_PullAlong>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_PullAlong::WParticleBehaviorFactory_PullAlong() = default;

const WRTTI* WParticleBehaviorFactory_PullAlong::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_PullAlong>();
}

void WParticleBehaviorFactory_PullAlong::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_PullAlong* pBehavior = static_cast<WParticleBehavior_PullAlong*>(pObject);

  pBehavior->m_fStrength = WMath::Clamp(m_fStrength, 0.0f, 1.0f);
}

enum class BehaviorPullAlongVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_PullAlong::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorPullAlongVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fStrength;
}

void WParticleBehaviorFactory_PullAlong::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorPullAlongVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fStrength;
}

void WParticleBehavior_PullAlong::CreateRequiredStreams()
{
  m_bFirstTime = true;
  m_vApplyPull.SetZero();

  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void WParticleBehavior_PullAlong::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: PullAlong");

  if (m_vApplyPull.IsZero())
    return;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  WSimdVec4f pull;
  pull.Load<3>(&m_vApplyPull.x);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += pull;

    itPosition.Advance();
  }
}

void WParticleBehavior_PullAlong::StepParticleSystem(const WTime& tDiff, WUInt32 uiNumNewParticles)
{
  const WVec3 vPos = GetOwnerSystem()->GetTransform().m_vPosition;

  if (!m_bFirstTime)
  {
    m_vApplyPull = (vPos - m_vLastEmitterPosition) * m_fStrength;
  }
  else
  {
    m_bFirstTime = false;
    m_vApplyPull.SetZero();
  }

  m_vLastEmitterPosition = vPos;
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_PullAlong);

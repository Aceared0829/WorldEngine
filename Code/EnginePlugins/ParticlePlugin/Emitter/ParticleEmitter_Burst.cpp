#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Burst.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitterFactory_Burst, 1, WRTTIDefaultAllocator<WParticleEmitterFactory_Burst>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Duration", m_Duration),
    W_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    W_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new WDefaultValueAttribute(10)),
    W_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    W_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitter_Burst, 1, WRTTIDefaultAllocator<WParticleEmitter_Burst>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEmitterFactory_Burst::WParticleEmitterFactory_Burst()
{
  m_uiSpawnCountMin = 10;
  m_uiSpawnCountRange = 0;
}

const WRTTI* WParticleEmitterFactory_Burst::GetEmitterType() const
{
  return WGetStaticRTTI<WParticleEmitter_Burst>();
}

void WParticleEmitterFactory_Burst::CopyEmitterProperties(WParticleEmitter* pEmitter0, bool bFirstTime) const
{
  WParticleEmitter_Burst* pEmitter = static_cast<WParticleEmitter_Burst*>(pEmitter0);

  pEmitter->m_Duration = m_Duration;
  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountMin = (WUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (WUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_sSpawnCountScaleParameter = WTempHashedString(m_sSpawnCountScaleParameter.GetData());
}


void WParticleEmitterFactory_Burst::QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = m_uiSpawnCountMin + m_uiSpawnCountRange;
  out_uiMaxParticlesPerSecond = 0;

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterBurstVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void WParticleEmitterFactory_Burst::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)EmitterBurstVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_Duration;
  inout_stream << m_StartDelay;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void WParticleEmitterFactory_Burst::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)EmitterBurstVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_Duration;
  inout_stream >> m_StartDelay;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

void WParticleEmitter_Burst::OnFinalize()
{
  float fSpawnFactor = 1.0f;

  const float spawnCountScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  WRandom& rng = GetRNG();

  m_uiSpawnCountLeft = (WUInt32)(rng.IntMinMax(m_uiSpawnCountMin, m_uiSpawnCountMin + m_uiSpawnCountRange) * fSpawnFactor);

  m_fSpawnAccu = 0;
  m_fSpawnPerSecond = 0;

  if (!m_Duration.IsZero())
  {
    m_fSpawnPerSecond = m_uiSpawnCountLeft / (float)m_Duration.GetSeconds();
  }
}

WParticleEmitterState WParticleEmitter_Burst::IsFinished()
{
  return (m_uiSpawnCountLeft == 0) ? WParticleEmitterState::Finished : WParticleEmitterState::Active;
}

WUInt32 WParticleEmitter_Burst::ComputeSpawnCount(const WTime& tDiff)
{
  W_PROFILE_SCOPE("PFX: Burst - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  WUInt32 uiSpawn = 0;

  if (m_Duration.IsZero())
  {
    uiSpawn = m_uiSpawnCountLeft;
    m_uiSpawnCountLeft = 0;
  }
  else
  {
    m_fSpawnAccu += (float)tDiff.GetSeconds() * m_fSpawnPerSecond;
    uiSpawn = (WUInt32)m_fSpawnAccu;
    uiSpawn = WMath::Min(uiSpawn, m_uiSpawnCountLeft);

    m_fSpawnAccu -= uiSpawn;
    m_uiSpawnCountLeft -= uiSpawn;
  }

  return uiSpawn;
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Burst);

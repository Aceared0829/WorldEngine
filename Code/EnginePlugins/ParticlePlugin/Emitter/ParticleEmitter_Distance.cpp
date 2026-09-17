#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Distance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitterFactory_Distance, 1, WRTTIDefaultAllocator<WParticleEmitterFactory_Distance>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DistanceThreshold", m_fDistanceThreshold)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.01f, 100.0f)),
    W_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new WDefaultValueAttribute(1)),
    W_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    W_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitter_Distance, 1, WRTTIDefaultAllocator<WParticleEmitter_Distance>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEmitterFactory_Distance::WParticleEmitterFactory_Distance() = default;

const WRTTI* WParticleEmitterFactory_Distance::GetEmitterType() const
{
  return WGetStaticRTTI<WParticleEmitter_Distance>();
}

void WParticleEmitterFactory_Distance::CopyEmitterProperties(WParticleEmitter* pEmitter0, bool bFirstTime) const
{
  WParticleEmitter_Distance* pEmitter = static_cast<WParticleEmitter_Distance*>(pEmitter0);

  pEmitter->m_fDistanceThresholdSQR = WMath::Square(m_fDistanceThreshold);

  pEmitter->m_uiSpawnCountMin = (WUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (WUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = WTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void WParticleEmitterFactory_Distance::QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 10; // assume that this won't fire more than 10 times per second

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void WParticleEmitterFactory_Distance::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)EmitterDistanceVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_fDistanceThreshold;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void WParticleEmitterFactory_Distance::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)EmitterDistanceVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fDistanceThreshold;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

void WParticleEmitter_Distance::CreateRequiredStreams() {}
void WParticleEmitter_Distance::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) {}

bool WParticleEmitter_Distance::IsContinuous() const
{
  return true;
}

void WParticleEmitter_Distance::OnFinalize()
{
  // do not use the System transform, because then this would not work with local space simulation
  m_vLastSpawnPosition = GetOwnerEffect()->GetTransform().m_vPosition;
  m_bFirstUpdate = true;

  if (GetOwnerEffect()->IsSharedEffect())
  {
    WLog::Warning("Particle emitters of type 'Distance' do not work for shared particle effect instances.");
  }
}

WParticleEmitterState WParticleEmitter_Distance::IsFinished()
{
  return WParticleEmitterState::Active;
}

WUInt32 WParticleEmitter_Distance::ComputeSpawnCount(const WTime& tDiff)
{
  const WVec3 vCurPos = GetOwnerEffect()->GetTransform().m_vPosition;

  if ((m_vLastSpawnPosition - vCurPos).GetLengthSquared() < m_fDistanceThresholdSQR)
    return 0;

  m_vLastSpawnPosition = vCurPos;

  if (m_bFirstUpdate)
  {
    m_bFirstUpdate = false;
    return 0;
  }

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  WUInt32 uiSpawn = m_uiSpawnCountMin;

  if (m_uiSpawnCountRange > 0)
    uiSpawn += GetRNG().UIntInRange(m_uiSpawnCountRange);

  uiSpawn = static_cast<WUInt32>((float)uiSpawn * fSpawnFactor);

  return uiSpawn;
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Distance);

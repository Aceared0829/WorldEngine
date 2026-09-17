#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitterFactory_Continuous, 1, WRTTIDefaultAllocator<WParticleEmitterFactory_Continuous>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    W_MEMBER_PROPERTY("SpawnCountPerSec", m_uiSpawnCountPerSec)->AddAttributes(new WDefaultValueAttribute(10)),
    W_MEMBER_PROPERTY("SpawnCountPerSecRange", m_uiSpawnCountPerSecRange),
    W_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),

    W_RESOURCE_MEMBER_PROPERTY("CountCurve", m_hCountCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("CurveDuration", m_CurveDuration)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromSeconds(10.0))),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitter_Continuous, 1, WRTTIDefaultAllocator<WParticleEmitter_Continuous>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEmitterFactory_Continuous::WParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountPerSec = 10;
  m_uiSpawnCountPerSecRange = 0;

  m_CurveDuration = WTime::MakeFromSeconds(10.0);
}


const WRTTI* WParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return WGetStaticRTTI<WParticleEmitter_Continuous>();
}

void WParticleEmitterFactory_Continuous::CopyEmitterProperties(WParticleEmitter* pEmitter0, bool bFirstTime) const
{
  WParticleEmitter_Continuous* pEmitter = static_cast<WParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountPerSec = (WUInt32)(m_uiSpawnCountPerSec * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountPerSecRange = (WUInt32)(m_uiSpawnCountPerSecRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = WTempHashedString(m_sSpawnCountScaleParameter.GetData());

  pEmitter->m_hCountCurve = m_hCountCurve;
  pEmitter->m_CurveDuration = WMath::Max(m_CurveDuration, WTime::MakeFromSeconds(1.0));
}

void WParticleEmitterFactory_Continuous::QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = m_uiSpawnCountPerSec + (m_uiSpawnCountPerSecRange * 3 / 4); // don't be too pessimistic

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterContinuousVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added emitter start delay
  Version_5, // added spawn count scale param
  Version_6, // removed duration, switched to particles per second

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void WParticleEmitterFactory_Continuous::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)EmitterContinuousVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 4
  inout_stream << m_StartDelay;

  // Version 6
  inout_stream << m_uiSpawnCountPerSec;
  inout_stream << m_uiSpawnCountPerSecRange;

  // Version 2
  inout_stream << m_hCountCurve;
  inout_stream << m_CurveDuration;

  // Version 5
  inout_stream << m_sSpawnCountScaleParameter;
}

void WParticleEmitterFactory_Continuous::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)EmitterContinuousVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 3 && uiVersion < 6)
  {
    WTime duraton;
    inout_stream >> duraton;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_StartDelay;
  }

  inout_stream >> m_uiSpawnCountPerSec;
  inout_stream >> m_uiSpawnCountPerSecRange;

  if (uiVersion < 6)
  {
    WVarianceTypeFloat interval;
    inout_stream >> interval.m_Value;
    inout_stream >> interval.m_fVariance;
  }

  if (uiVersion >= 2)
  {
    inout_stream >> m_hCountCurve;
    inout_stream >> m_CurveDuration;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

void WParticleEmitter_Continuous::OnFinalize()
{
  m_CountCurveTime = WTime::MakeZero();
  m_fCurSpawnPerSec = static_cast<float>(GetRNG().DoubleMinMax(m_uiSpawnCountPerSec, m_uiSpawnCountPerSec + m_uiSpawnCountPerSecRange));
  m_TimeSinceRandom = WTime::MakeZero();
  m_fCurSpawnCounter = 1; // make sure to always spawn at least one particle right away in the first frame
}

WParticleEmitterState WParticleEmitter_Continuous::IsFinished()
{
  return WParticleEmitterState::Active;
}

WUInt32 WParticleEmitter_Continuous::ComputeSpawnCount(const WTime& tDiff)
{
  W_PROFILE_SCOPE("PFX: Continuous - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  m_TimeSinceRandom += tDiff;
  m_CountCurveTime += tDiff;

  if (m_TimeSinceRandom >= WTime::MakeFromMilliseconds(200))
  {
    m_TimeSinceRandom = WTime::MakeZero();
    m_fCurSpawnPerSec = (float)GetRNG().DoubleMinMax(m_uiSpawnCountPerSec, m_uiSpawnCountPerSec + m_uiSpawnCountPerSecRange);
  }


  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurve(m_hCountCurve, WResourceAcquireMode::BlockTillLoaded);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const double normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const double evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = (float)WMath::Max(0.0, curve.Evaluate(evalPos));
    }
  }

  const float spawnCountScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  m_fCurSpawnCounter += fSpawnFactor * m_fCurSpawnPerSec * (float)tDiff.GetSeconds();

  const WUInt32 uiSpawn = (WUInt32)m_fCurSpawnCounter;
  m_fCurSpawnCounter -= uiSpawn;

  return uiSpawn;
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);

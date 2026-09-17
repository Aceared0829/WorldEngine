#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitterFactory_OnEvent, 1, WRTTIDefaultAllocator<WParticleEmitterFactory_OnEvent>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EventName", m_sEventName)->AddAttributes(new WDynamicStringEnumAttribute("ParticleEventNamesEnum")),
    W_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new WDefaultValueAttribute(1)),
    W_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    W_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEmitter_OnEvent, 1, WRTTIDefaultAllocator<WParticleEmitter_OnEvent>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEmitterFactory_OnEvent::WParticleEmitterFactory_OnEvent() = default;
WParticleEmitterFactory_OnEvent::~WParticleEmitterFactory_OnEvent() = default;

const WRTTI* WParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return WGetStaticRTTI<WParticleEmitter_OnEvent>();
}

void WParticleEmitterFactory_OnEvent::CopyEmitterProperties(WParticleEmitter* pEmitter0, bool bFirstTime) const
{
  WParticleEmitter_OnEvent* pEmitter = static_cast<WParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = WTempHashedString(m_sEventName.GetData());

  pEmitter->m_uiSpawnCountMin = (WUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (WUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = WTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void WParticleEmitterFactory_OnEvent::QueryMaxParticleCount(WUInt32& out_uiMaxParticlesAbs, WUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 16; // some wild guess

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterOnEventVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void WParticleEmitterFactory_OnEvent::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)EmitterOnEventVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventName;

  // Version 2
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void WParticleEmitterFactory_OnEvent::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)EmitterOnEventVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sEventName;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiSpawnCountMin;
    inout_stream >> m_uiSpawnCountRange;
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

WParticleEmitterState WParticleEmitter_OnEvent::IsFinished()
{
  return WParticleEmitterState::OnlyReacting;
}

WUInt32 WParticleEmitter_OnEvent::ComputeSpawnCount(const WTime& tDiff)
{
  if (!m_bSpawn)
    return 0;

  m_bSpawn = false;

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  return static_cast<WUInt32>((m_uiSpawnCountMin + GetRNG().UIntInRange(1 + m_uiSpawnCountRange)) * fSpawnFactor);
}

void WParticleEmitter_OnEvent::ProcessEventQueue(WParticleEventQueue queue)
{
  if (m_bSpawn)
    return;

  for (const WParticleEvent& e : queue)
  {
    if (e.m_EventType == m_sEventName) // this is the event type we are waiting for!
    {
      m_bSpawn = true;
      return;
    }
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_OnEvent);

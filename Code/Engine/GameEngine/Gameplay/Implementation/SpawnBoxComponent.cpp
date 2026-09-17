#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/SpawnBoxComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSpawnBoxComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(2.0f, 2.0f, 0.25f)), new WClampValueAttribute(WVec3(0), WVariant())),
    W_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    W_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    W_MEMBER_PROPERTY("MinSpawnCount", m_uiMinSpawnCount)->AddAttributes(new WDefaultValueAttribute(10)),
    W_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange)->AddAttributes(new WDefaultValueAttribute(0)),
    W_MEMBER_PROPERTY("Duration", m_SpawnDuration)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromSeconds(5))),
    W_MEMBER_PROPERTY("MaxRotationZ", m_MaxRotationZ),
    W_MEMBER_PROPERTY("MaxTiltZ", m_MaxTiltZ),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new WBoxVisualizerAttribute("HalfExtents", 2.0f),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.5f, WColorScheme::LightUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(StartSpawning),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE;
// clang-format on

void WSpawnBoxComponent::SetHalfExtents(const WVec3& value)
{
  m_vHalfExtents = value.CompMax(WVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool WSpawnBoxComponent::GetSpawnAtStart() const
{
  return m_Flags.IsAnySet(WSpawnBoxComponentFlags::SpawnAtStart);
}

void WSpawnBoxComponent::SetSpawnAtStart(bool b)
{
  m_Flags.AddOrRemove(WSpawnBoxComponentFlags::SpawnAtStart, b);
}

bool WSpawnBoxComponent::GetSpawnContinuously() const
{
  return m_Flags.IsAnySet(WSpawnBoxComponentFlags::SpawnContinuously);
}

void WSpawnBoxComponent::SetSpawnContinuously(bool b)
{
  m_Flags.AddOrRemove(WSpawnBoxComponentFlags::SpawnContinuously, b);
}

void WSpawnBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
  s << m_hPrefab;
  s << m_Flags;
  s << m_SpawnDuration;
  s << m_uiMinSpawnCount;
  s << m_uiSpawnCountRange;
  s << m_MaxRotationZ;
  s << m_MaxTiltZ;
}

void WSpawnBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
  s >> m_hPrefab;
  s >> m_Flags;
  s >> m_SpawnDuration;
  s >> m_uiMinSpawnCount;
  s >> m_uiSpawnCountRange;
  s >> m_MaxRotationZ;
  s >> m_MaxTiltZ;
}

void WSpawnBoxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetSpawnAtStart())
  {
    StartSpawning();
  }
}

void WSpawnBoxComponent::StartSpawning()
{
  InternalStartSpawning(true);
}

void WSpawnBoxComponent::InternalStartSpawning(bool bFirstTime)
{

  m_uiSpawned = 0;
  m_uiTotalToSpawn = m_uiMinSpawnCount;
  m_StartTime = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_uiSpawnCountRange > 0)
  {
    m_uiTotalToSpawn = GetWorld()->GetRandomNumberGenerator().IntMinMax(m_uiMinSpawnCount, m_uiMinSpawnCount + m_uiSpawnCountRange);
  }

  if (m_uiTotalToSpawn == 0)
    return;

  if (m_SpawnDuration.IsZeroOrNegative())
  {
    Spawn(m_uiTotalToSpawn);
  }
  else
  {
    if (bFirstTime)
    {
      // this guarantees that next time OnTriggered() is called, one object gets spawned right away
      m_StartTime -= m_SpawnDuration / m_uiTotalToSpawn;
    }

    WMsgComponentInternalTrigger msg;
    PostMessage(msg, WTime::MakeZero());
  }
}

void WSpawnBoxComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  const WTime tActive = tNow - m_StartTime;
  const WTime tEnd = m_StartTime + m_SpawnDuration;

  if (tNow >= tEnd)
  {
    if (m_uiSpawned < m_uiTotalToSpawn)
    {
      Spawn(m_uiTotalToSpawn - m_uiSpawned);
    }

    if (GetSpawnContinuously())
    {
      InternalStartSpawning(false);
    }

    return;
  }

  const auto uiTargetSpawnCount = WMath::Clamp<WUInt16>(static_cast<WUInt16>(((tActive.GetSeconds() / m_SpawnDuration.GetSeconds()) * m_uiTotalToSpawn)), 0, m_uiTotalToSpawn);

  if (m_uiSpawned < uiTargetSpawnCount)
  {
    Spawn(uiTargetSpawnCount - m_uiSpawned);
  }

  if (m_uiSpawned < m_uiTotalToSpawn)
  {
    // remaining time divided equally for the remaining spawns
    // this is to prevent a lot of unnecessary message sending at low spawn counts
    WTime tDelay = (tEnd - tNow) / (m_uiTotalToSpawn - m_uiSpawned);

    // prevent unnecessary high number of updates, rather spawn multiple objects within one frame
    tDelay = WMath::Max(tDelay, WTime::MakeFromMilliseconds(40)); // max 25 Hz

    WMsgComponentInternalTrigger msg;
    PostMessage(msg, tDelay);
  }
  else if (GetSpawnContinuously())
  {
    InternalStartSpawning(false);
  }
}

void WSpawnBoxComponent::Spawn(WUInt32 uiCount)
{
  if (uiCount == 0)
    return;

  m_uiSpawned += uiCount;

  if (!m_hPrefab.IsValid())
    return;

  WResourceLock<WPrefabResource> pResource(m_hPrefab, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pResource.GetAcquireResult() == WResourceAcquireResult::None)
    return;

  WPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  WRandom& rnd = GetWorld()->GetRandomNumberGenerator();
  const WTransform tOwner = GetOwner()->GetGlobalTransform();

  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    WTransform tLocal = WTransform::MakeIdentity();
    tLocal.m_vPosition.x = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.x, m_vHalfExtents.x));
    tLocal.m_vPosition.y = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.y, m_vHalfExtents.y));
    tLocal.m_vPosition.z = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.z, m_vHalfExtents.z));

    if (m_MaxRotationZ.GetRadian() > 0)
    {
      const WAngle rotationAngle = WAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(-m_MaxRotationZ.GetRadian(), +m_MaxRotationZ.GetRadian()));
      const WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), rotationAngle);

      tLocal.m_qRotation = qRot;
    }

    if (m_MaxTiltZ.GetRadian() > 0)
    {
      const WAngle tiltTurnAngle = WAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, WMath::Pi<double>() * 2.0));
      const WQuat qTiltTurn = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), tiltTurnAngle);

      const WVec3 vTiltAxis = qTiltTurn * WVec3(1, 0, 0);

      const WAngle tiltAngle = WAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, (double)m_MaxTiltZ.GetRadian()));
      const WQuat qTilt = WQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);

      tLocal.m_qRotation = tLocal.m_qRotation * qTilt;
    }

    const WTransform tGlobal = WTransform::MakeGlobalTransform(tOwner, tLocal);

    pResource->InstantiatePrefab(*GetWorld(), tGlobal, options);
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnBoxComponent);

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/SpawnComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSpawnComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package), new WRequiredAttribute()),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("Prefab")),
    W_ACCESSOR_PROPERTY("AttachAsChild", GetAttachAsChild, SetAttachAsChild),
    W_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    W_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    W_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new WClampValueAttribute(WTime(), WVariant()), new WDefaultValueAttribute(WTime::MakeFromSeconds(1.0))),
    W_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new WClampValueAttribute(WTime(), WVariant())),
    W_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(179.0))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.5f, WColorScheme::LightUI(WColorScheme::Lime)),
    new WConeVisualizerAttribute(WBasisAxis::PositiveX, "Deviation", 0.5f, nullptr, WColorScheme::LightUI(WColorScheme::Lime)),
    new WConeAngleManipulatorAttribute("Deviation", 0.5f),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(CanTriggerManualSpawn),
    W_SCRIPT_FUNCTION_PROPERTY(TriggerManualSpawn, In, "IgnoreSpawnDelay", In, "LocalOffset"),
    W_SCRIPT_FUNCTION_PROPERTY(ScheduleSpawn),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSpawnComponent::WSpawnComponent() = default;
WSpawnComponent::~WSpawnComponent() = default;

void WSpawnComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_SpawnFlags.IsAnySet(WSpawnComponentFlags::SpawnAtStart))
  {
    ScheduleSpawn();
  }
}


void WSpawnComponent::OnDeactivated()
{
  m_SpawnFlags.Remove(WSpawnComponentFlags::SpawnInFlight);

  SUPER::OnDeactivated();
}

bool WSpawnComponent::SpawnOnce(const WVec3& vLocalOffset)
{
  if (m_hPrefab.IsValid())
  {
    WTransform tLocalSpawn;
    tLocalSpawn.SetIdentity();
    tLocalSpawn.m_vPosition = vLocalOffset;

    if (m_MaxDeviation.GetRadian() > 0)
    {
      const WVec3 vTiltAxis = WVec3(0, 1, 0);
      const WVec3 vTurnAxis = WVec3(1, 0, 0);

      const WAngle tiltAngle = WAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, (double)m_MaxDeviation.GetRadian()));
      const WAngle turnAngle = WAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, WMath::Pi<double>() * 2.0));

      WQuat qTilt, qTurn, qDeviate;
      qTilt = WQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);
      qTurn = WQuat::MakeFromAxisAndAngle(vTurnAxis, turnAngle);
      qDeviate = qTurn * qTilt;

      tLocalSpawn.m_qRotation = qDeviate;
    }

    DoSpawn(tLocalSpawn);

    return true;
  }

  return false;
}


void WSpawnComponent::DoSpawn(const WTransform& tLocalSpawn)
{
  WResourceLock<WPrefabResource> pResource(m_hPrefab, WResourceAcquireMode::AllowLoadingFallback);

  WPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  if (m_SpawnFlags.IsAnySet(WSpawnComponentFlags::AttachAsChild))
  {
    options.m_hParent = GetOwner()->GetHandle();

    pResource->InstantiatePrefab(*GetWorld(), tLocalSpawn, options, &m_Parameters);
  }
  else
  {
    WTransform tGlobalSpawn;
    tGlobalSpawn = WTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), tLocalSpawn);
    tGlobalSpawn.m_vScale.Set(1);

    pResource->InstantiatePrefab(*GetWorld(), tGlobalSpawn, options, &m_Parameters);
  }
}

void WSpawnComponent::ScheduleSpawn()
{
  if (m_SpawnFlags.IsAnySet(WSpawnComponentFlags::SpawnInFlight))
    return;

  WMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("scheduled_spawn");

  m_SpawnFlags.Add(WSpawnComponentFlags::SpawnInFlight);

  WWorld* pWorld = GetWorld();

  const WTime tKill = WTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleMinMax(m_MinDelay.GetSeconds(), m_MinDelay.GetSeconds() + m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);
}

void WSpawnComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_SpawnFlags.GetValue();
  s << m_hPrefab;

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_MaxDeviation;
  s << m_LastManualSpawn;

  WPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void WSpawnComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  WSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_SpawnFlags.SetValue(flags);

  s >> m_hPrefab;

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_MaxDeviation;
  s >> m_LastManualSpawn;

  if (uiVersion >= 3)
  {
    WPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

bool WSpawnComponent::CanTriggerManualSpawn() const
{
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  return tNow - m_LastManualSpawn >= m_MinDelay;
}

bool WSpawnComponent::TriggerManualSpawn(bool bIgnoreSpawnDelay /*= false*/, const WVec3& vLocalOffset /*= WVec3::MakeZero()*/)
{
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (bIgnoreSpawnDelay == false && tNow - m_LastManualSpawn < m_MinDelay)
    return false;

  m_LastManualSpawn = tNow;
  return SpawnOnce(vLocalOffset);
}

bool WSpawnComponent::GetSpawnAtStart() const
{
  return m_SpawnFlags.IsAnySet(WSpawnComponentFlags::SpawnAtStart);
}

void WSpawnComponent::SetSpawnAtStart(bool b)
{
  m_SpawnFlags.AddOrRemove(WSpawnComponentFlags::SpawnAtStart, b);
}

bool WSpawnComponent::GetSpawnContinuously() const
{
  return m_SpawnFlags.IsAnySet(WSpawnComponentFlags::SpawnContinuously);
}

void WSpawnComponent::SetSpawnContinuously(bool b)
{
  m_SpawnFlags.AddOrRemove(WSpawnComponentFlags::SpawnContinuously, b);
}

bool WSpawnComponent::GetAttachAsChild() const
{
  return m_SpawnFlags.IsAnySet(WSpawnComponentFlags::AttachAsChild);
}

void WSpawnComponent::SetAttachAsChild(bool b)
{
  m_SpawnFlags.AddOrRemove(WSpawnComponentFlags::AttachAsChild, b);
}

void WSpawnComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == WTempHashedString("scheduled_spawn"))
  {
    m_SpawnFlags.Remove(WSpawnComponentFlags::SpawnInFlight);

    SpawnOnce(WVec3::MakeZero());

    // do it all again
    if (m_SpawnFlags.IsAnySet(WSpawnComponentFlags::SpawnContinuously))
    {
      ScheduleSpawn();
    }
  }
  else if (msg.m_sMessage == WTempHashedString("spawn"))
  {
    TriggerManualSpawn();
  }
}

const WRangeView<const char*, WUInt32> WSpawnComponent::GetParameters() const
{
  return WRangeView<const char*, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_Parameters.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void WSpawnComponent::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void WSpawnComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(WTempHashedString(szKey));
}

bool WSpawnComponent::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WSpawnComponentPatch_1_2 : public WGraphPatch
{
public:
  WSpawnComponentPatch_1_2()
    : WGraphPatch("WSpawnComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Attach as Child", "AttachAsChild");
    pNode->RenameProperty("Spawn at Start", "SpawnAtStart");
    pNode->RenameProperty("Spawn Continuously", "SpawnContinuously");
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
  }
};

WSpawnComponentPatch_1_2 g_WSpawnComponentPatch_1_2;



W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnComponent);

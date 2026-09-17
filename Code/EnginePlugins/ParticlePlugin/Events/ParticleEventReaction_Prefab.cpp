#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReactionFactory_Prefab, 1, WRTTIDefaultAllocator<WParticleEventReactionFactory_Prefab>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab"), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("Alignment", WSurfaceInteractionAlignment, m_Alignment),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReaction_Prefab, 1, WRTTIDefaultAllocator<WParticleEventReaction_Prefab>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEventReactionFactory_Prefab::WParticleEventReactionFactory_Prefab() = default;

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleEventReactionFactory_Prefab::Save(WStreamWriter& inout_stream) const
{
  SUPER::Save(inout_stream);

  const WUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sPrefab;

  // Version 2
  inout_stream << m_Alignment;
}

void WParticleEventReactionFactory_Prefab::Load(WStreamReader& inout_stream)
{
  SUPER::Load(inout_stream);

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    inout_stream >> m_Alignment;
  }

  if (!m_sPrefab.IsEmpty())
  {
    m_hPrefab = WResourceManager::LoadResource<WPrefabResource>(m_sPrefab);
  }
}


const WRTTI* WParticleEventReactionFactory_Prefab::GetEventReactionType() const
{
  return WGetStaticRTTI<WParticleEventReaction_Prefab>();
}


void WParticleEventReactionFactory_Prefab::CopyReactionProperties(WParticleEventReaction* pObject, bool bFirstTime) const
{
  WParticleEventReaction_Prefab* pReaction = static_cast<WParticleEventReaction_Prefab*>(pObject);

  pReaction->m_hPrefab = m_hPrefab;
  pReaction->m_Alignment = m_Alignment;
}

//////////////////////////////////////////////////////////////////////////

WParticleEventReaction_Prefab::WParticleEventReaction_Prefab() = default;
WParticleEventReaction_Prefab::~WParticleEventReaction_Prefab() = default;

void WParticleEventReaction_Prefab::ProcessEvent(const WParticleEvent& e)
{
  if (!m_hPrefab.IsValid())
    return;

  WTransform trans;
  trans.m_vScale.Set(1.0f);
  trans.m_vPosition = e.m_vPosition;

  WVec3 vAlignDir = e.m_vNormal;

  switch (m_Alignment)
  {
    case WSurfaceInteractionAlignment::IncidentDirection:
      vAlignDir = -e.m_vDirection;
      break;

    case WSurfaceInteractionAlignment::ReflectedDirection:
      vAlignDir = e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case WSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vAlignDir = -e.m_vNormal;
      break;

    case WSurfaceInteractionAlignment::ReverseIncidentDirection:
      vAlignDir = e.m_vDirection;
      ;
      break;

    case WSurfaceInteractionAlignment::ReverseReflectedDirection:
      vAlignDir = -e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case WSurfaceInteractionAlignment::SurfaceNormal:
      break;
  }

  // rotate the prefab randomly along its main axis (the X axis)
  WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromRadian((float)m_pOwnerEffect->GetRNG().DoubleZeroToOneInclusive() * WMath::Pi<float>() * 2.0f));

  vAlignDir.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();

  trans.m_qRotation = WQuat::MakeShortestRotation(WVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  WResourceLock<WPrefabResource> pPrefab(m_hPrefab, WResourceAcquireMode::BlockTillLoaded);

  WPrefabInstantiationOptions options;

  pPrefab->InstantiatePrefab(*m_pOwnerEffect->GetWorld(), trans, options);
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction_Prefab);

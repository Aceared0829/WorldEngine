#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Effect.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReactionFactory_Effect, 1, WRTTIDefaultAllocator<WParticleEventReactionFactory_Effect>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Particle_Effect"), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("Alignment", WSurfaceInteractionAlignment, m_Alignment),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("Effect"), new WExposeColorAlphaAttribute),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReaction_Effect, 1, WRTTIDefaultAllocator<WParticleEventReaction_Effect>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEventReactionFactory_Effect::WParticleEventReactionFactory_Effect()
{
  m_pParameters = W_DEFAULT_NEW(WParticleEffectParameters);
}

enum class ReactionEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added effect parameters
  Version_3, // added alignment

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleEventReactionFactory_Effect::Save(WStreamWriter& inout_stream) const
{
  SUPER::Save(inout_stream);

  const WUInt8 uiVersion = (int)ReactionEffectVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEffect;

  // Version 2
  inout_stream << m_pParameters->m_FloatParams.GetCount();
  for (WUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    inout_stream << m_pParameters->m_FloatParams[i].m_sName;
    inout_stream << m_pParameters->m_FloatParams[i].m_Value;
  }
  inout_stream << m_pParameters->m_ColorParams.GetCount();
  for (WUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    inout_stream << m_pParameters->m_ColorParams[i].m_sName;
    inout_stream << m_pParameters->m_ColorParams[i].m_Value;
  }

  // Version 3
  inout_stream << m_Alignment;
}

void WParticleEventReactionFactory_Effect::Load(WStreamReader& inout_stream)
{
  SUPER::Load(inout_stream);

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)ReactionEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    WUInt32 numFloats, numColors;

    inout_stream >> numFloats;
    m_pParameters->m_FloatParams.SetCountUninitialized(numFloats);

    for (WUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      inout_stream >> m_pParameters->m_FloatParams[i].m_sName;
      inout_stream >> m_pParameters->m_FloatParams[i].m_Value;
    }

    inout_stream >> numColors;
    m_pParameters->m_ColorParams.SetCountUninitialized(numColors);

    for (WUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      inout_stream >> m_pParameters->m_ColorParams[i].m_sName;
      inout_stream >> m_pParameters->m_ColorParams[i].m_Value;
    }
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_Alignment;
  }

  if (!m_sEffect.IsEmpty())
  {
    m_hEffect = WResourceManager::LoadResource<WParticleEffectResource>(m_sEffect);
  }
}


const WRTTI* WParticleEventReactionFactory_Effect::GetEventReactionType() const
{
  return WGetStaticRTTI<WParticleEventReaction_Effect>();
}


void WParticleEventReactionFactory_Effect::CopyReactionProperties(WParticleEventReaction* pObject, bool bFirstTime) const
{
  WParticleEventReaction_Effect* pReaction = static_cast<WParticleEventReaction_Effect*>(pObject);

  pReaction->m_hEffect = m_hEffect;
  pReaction->m_Alignment = m_Alignment;

  pReaction->m_Parameters = m_pParameters;
}

const WRangeView<const char*, WUInt32> WParticleEventReactionFactory_Effect::GetParameters() const
{
  return WRangeView<const char*, WUInt32>([this]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_pParameters->m_FloatParams.GetCount() + m_pParameters->m_ColorParams.GetCount(); }, [this](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    {
      if (uiIt < m_pParameters->m_FloatParams.GetCount())
        return m_pParameters->m_FloatParams[uiIt].m_sName.GetData();
      else
        return m_pParameters->m_ColorParams[uiIt - m_pParameters->m_FloatParams.GetCount()].m_sName.GetData();
    });
}

void WParticleEventReactionFactory_Effect::SetParameter(const char* szKey, const WVariant& var)
{
  const WTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (WUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      if (m_pParameters->m_FloatParams[i].m_sName == th)
      {
        if (m_pParameters->m_FloatParams[i].m_Value != value)
        {
          m_pParameters->m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<WColor>())
  {
    WColor value = var.ConvertTo<WColor>();

    for (WUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      if (m_pParameters->m_ColorParams[i].m_sName == th)
      {
        if (m_pParameters->m_ColorParams[i].m_Value != value)
        {
          m_pParameters->m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void WParticleEventReactionFactory_Effect::RemoveParameter(const char* szKey)
{
  const WTempHashedString th(szKey);

  for (WUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    if (m_pParameters->m_FloatParams[i].m_sName == th)
    {
      m_pParameters->m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (WUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    if (m_pParameters->m_ColorParams[i].m_sName == th)
    {
      m_pParameters->m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool WParticleEventReactionFactory_Effect::GetParameter(const char* szKey, WVariant& out_value) const
{
  const WTempHashedString th(szKey);

  for (const auto& e : m_pParameters->m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_pParameters->m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

WParticleEventReaction_Effect::WParticleEventReaction_Effect() = default;
WParticleEventReaction_Effect::~WParticleEventReaction_Effect() = default;

void WParticleEventReaction_Effect::ProcessEvent(const WParticleEvent& e)
{
  if (!m_hEffect.IsValid())
    return;

  WGameObjectDesc god;
  god.m_bDynamic = true;
  god.m_LocalPosition = e.m_vPosition;

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

  if (!vAlignDir.IsZero())
  {
    god.m_LocalRotation = WQuat::MakeShortestRotation(WVec3(0, 0, 1), vAlignDir);
  }

  WGameObject* pObject = nullptr;
  m_pOwnerEffect->GetWorld()->CreateObject(god, pObject);

  WParticleComponent* pComponent = nullptr;
  WParticleComponent::CreateComponent(pObject, pComponent);

  pComponent->m_uiRandomSeed = m_pOwnerEffect->GetRandomSeed();

  pComponent->m_bIfContinuousStopRightAway = true;
  pComponent->m_OnFinishedAction = WOnComponentFinishedAction2::DeleteGameObject;
  pComponent->SetParticleEffect(m_hEffect);

  if (!m_Parameters->m_FloatParams.IsEmpty())
  {
    pComponent->m_bFloatParamsChanged = true;
    pComponent->m_FloatParams = m_Parameters->m_FloatParams;
  }

  if (!m_Parameters->m_ColorParams.IsEmpty())
  {
    pComponent->m_bColorParamsChanged = true;
    pComponent->m_ColorParams = m_Parameters->m_ColorParams;
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction_Effect);

#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReactionFactory, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EventType", m_sEventType)->AddAttributes(new WDynamicStringEnumAttribute("ParticleEventNamesEnum")),
    W_MEMBER_PROPERTY("Probability", m_uiProbability)->AddAttributes(new WDefaultValueAttribute(100), new WClampValueAttribute(1, 100)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEventReaction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

WParticleEventReaction* WParticleEventReactionFactory::CreateEventReaction(WParticleEffectInstance* pOwner) const
{
  const WRTTI* pRtti = GetEventReactionType();

  WParticleEventReaction* pReaction = pRtti->GetAllocator()->Allocate<WParticleEventReaction>();
  pReaction->Reset(pOwner);
  pReaction->m_sEventName = WTempHashedString(m_sEventType.GetData());
  pReaction->m_uiProbability = m_uiProbability;

  CopyReactionProperties(pReaction, true);

  return pReaction;
}

enum class ReactionVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added probability

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleEventReactionFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventType;

  // Version 2
  inout_stream << m_uiProbability;
}


void WParticleEventReactionFactory::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sEventType;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiProbability;
  }
}

//////////////////////////////////////////////////////////////////////////

WParticleEventReaction::WParticleEventReaction() = default;
WParticleEventReaction::~WParticleEventReaction() = default;

void WParticleEventReaction::Reset(WParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction);

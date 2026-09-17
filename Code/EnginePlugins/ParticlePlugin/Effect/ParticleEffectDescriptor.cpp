#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEffectDescriptor, 2, WRTTIDefaultAllocator<WParticleEffectDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("WhenInvisible", WEffectInvisibleUpdateRate, m_InvisibleUpdateRate),
    W_MEMBER_PROPERTY("AlwaysShared", m_bAlwaysShared),
    W_MEMBER_PROPERTY("SimulateInLocalSpace", m_bSimulateInLocalSpace),
    W_MEMBER_PROPERTY("ApplyOwnerVelocity", m_fApplyInstanceVelocity)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("PreSimulateDuration", m_PreSimulateDuration),
    W_MEMBER_PROPERTY("NumWindSamples", m_vNumWindSamples)->AddAttributes(new WDefaultValueAttribute(WVec3U32(1)), new WClampValueAttribute(WVec3U32(1), WVec3U32(8))),
    W_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    W_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new WExposeColorAlphaAttribute),
    W_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(WPropertyFlags::PointerOwner),
    W_SET_ACCESSOR_PROPERTY("EventReactions", GetEventReactions, AddEventReaction, RemoveEventReaction)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEffectDescriptor::WParticleEffectDescriptor() = default;

WParticleEffectDescriptor::~WParticleEffectDescriptor()
{
  ClearSystems();
  ClearEventReactions();
}

void WParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}


void WParticleEffectDescriptor::ClearEventReactions()
{
  for (auto pReaction : m_EventReactions)
  {
    pReaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pReaction);
  }

  m_EventReactions.Clear();
}

enum class ParticleEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5,  // m_bAlwaysShared
  Version_6,  // added parameters
  Version_7,  // added instance velocity
  Version_8,  // added event reactions
  Version_9,  // breaking change
  Version_10, // added wind samples
  Version_11, // change serialization order

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleEffectDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  inout_stream << uiVersion;

  const WUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  inout_stream << uiNumSystems;

  // Version 3
  inout_stream << m_bSimulateInLocalSpace;
  inout_stream << m_PreSimulateDuration;
  // Version 4
  inout_stream << m_InvisibleUpdateRate;
  // Version 5
  inout_stream << m_bAlwaysShared;

  // Version 6
  {
    WUInt8 paramCol = static_cast<WUInt8>(m_ColorParameters.GetCount());
    inout_stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }

    WUInt8 paramFloat = static_cast<WUInt8>(m_FloatParameters.GetCount());
    inout_stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }
  }

  // Version 7
  inout_stream << m_fApplyInstanceVelocity;

  // Version 8
  {
    const WUInt32 uiNumReactions = m_EventReactions.GetCount();
    inout_stream << uiNumReactions;

    for (auto pReaction : m_EventReactions)
    {
      inout_stream << pReaction->GetDynamicRTTI()->GetTypeName();

      pReaction->Save(inout_stream);
    }
  }

  // Version 10
  inout_stream << m_vNumWindSamples;

  // Version 11 (moved to the end)
  for (auto pSystem : m_ParticleSystems)
  {
    inout_stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(inout_stream);
  }
}


void WParticleEffectDescriptor::Load(WStreamReader& inout_stream)
{
  ClearSystems();
  ClearEventReactions();

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  W_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion < (int)ParticleEffectVersion::Version_9)
  {
    WLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  WUInt32 uiNumSystems = 0;
  inout_stream >> uiNumSystems;

  inout_stream >> m_bSimulateInLocalSpace;
  inout_stream >> m_PreSimulateDuration;
  inout_stream >> m_InvisibleUpdateRate;
  inout_stream >> m_bAlwaysShared;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  WStringBuilder sType;

  if (uiVersion <= (int)ParticleEffectVersion::Version_10)
  {
    for (auto& pSystem : m_ParticleSystems)
    {
      inout_stream >> sType;

      const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
      W_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

      pSystem = pRtti->GetAllocator()->Allocate<WParticleSystemDescriptor>();

      pSystem->Load(inout_stream, *this);
    }
  }

  WStringBuilder key;
  m_ColorParameters.Clear();
  m_FloatParameters.Clear();

  WUInt8 paramCol;
  inout_stream >> paramCol;
  for (WUInt32 i = 0; i < paramCol; ++i)
  {
    WColor val;
    inout_stream >> key;
    inout_stream >> val;
    m_ColorParameters[key] = val;
  }

  WUInt8 paramFloat;
  inout_stream >> paramFloat;
  for (WUInt32 i = 0; i < paramFloat; ++i)
  {
    float val;
    inout_stream >> key;
    inout_stream >> val;
    m_FloatParameters[key] = val;
  }

  inout_stream >> m_fApplyInstanceVelocity;

  WUInt32 uiNumReactions = 0;
  inout_stream >> uiNumReactions;

  m_EventReactions.SetCountUninitialized(uiNumReactions);

  for (auto& pReaction : m_EventReactions)
  {
    inout_stream >> sType;

    const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
    W_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect event reaction type '{0}'", sType);

    pReaction = pRtti->GetAllocator()->Allocate<WParticleEventReactionFactory>();

    pReaction->Load(inout_stream);
  }

  if (uiVersion >= (int)ParticleEffectVersion::Version_10)
  {
    inout_stream >> m_vNumWindSamples;
  }

  // serialize the systems AFTER we know everything about the effect
  if (uiVersion >= (int)ParticleEffectVersion::Version_11)
  {
    for (auto& pSystem : m_ParticleSystems)
    {
      inout_stream >> sType;

      const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
      W_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

      pSystem = pRtti->GetAllocator()->Allocate<WParticleSystemDescriptor>();

      pSystem->Load(inout_stream, *this);
    }
  }
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);

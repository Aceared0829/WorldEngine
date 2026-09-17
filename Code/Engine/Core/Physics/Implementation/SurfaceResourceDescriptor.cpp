#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSurfaceInteractionAlignment, 2)
  W_ENUM_CONSTANTS(WSurfaceInteractionAlignment::SurfaceNormal, WSurfaceInteractionAlignment::IncidentDirection, WSurfaceInteractionAlignment::ReflectedDirection)
  W_ENUM_CONSTANTS(WSurfaceInteractionAlignment::ReverseSurfaceNormal, WSurfaceInteractionAlignment::ReverseIncidentDirection, WSurfaceInteractionAlignment::ReverseReflectedDirection)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WSurfaceInteraction, WNoBase, 1, WRTTIDefaultAllocator<WSurfaceInteraction>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new WDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    W_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package), new WRequiredAttribute()),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("Prefab")),
    W_ENUM_MEMBER_PROPERTY("Alignment", WSurfaceInteractionAlignment, m_Alignment),
    W_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new WClampValueAttribute(WVariant(WAngle::MakeFromDegree(0.0f)), WVariant(WAngle::MakeFromDegree(90.0f)))),
    W_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    W_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSurfaceResourceDescriptor, 3, WRTTIDefaultAllocator<WSurfaceResourceDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("BaseSurface", m_hBaseSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface")),// package+thumbnail so that it forbids circular dependencies
    W_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new WDefaultValueAttribute(0.25f)),
    W_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new WDefaultValueAttribute(0.6f)),
    W_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new WDefaultValueAttribute(0.4f)),
    W_MEMBER_PROPERTY("GroundType", m_iGroundType)->AddAttributes(new WDefaultValueAttribute(-1), new WDynamicEnumAttribute("AiGroundType")),
    W_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new WDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    W_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
    W_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("DebugColor", m_DebugColor),
    W_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRangeView<const char*, WUInt32> WSurfaceInteraction::GetParameters() const
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

void WSurfaceInteraction::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void WSurfaceInteraction::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(WTempHashedString(szKey));
}

bool WSurfaceInteraction::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void WSurfaceResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  W_ASSERT_DEV(uiVersion <= 9, "Invalid version {0} for surface resource", uiVersion);

  inout_stream >> m_fPhysicsRestitution;
  inout_stream >> m_fPhysicsFrictionStatic;
  inout_stream >> m_fPhysicsFrictionDynamic;
  inout_stream >> m_hBaseSurface;

  if (uiVersion >= 4)
  {
    inout_stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sSlideInteractionPrefab;
    inout_stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    WUInt32 count = 0;
    inout_stream >> count;
    m_Interactions.SetCount(count);

    WStringBuilder sTemp;
    for (WUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      inout_stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      inout_stream >> ia.m_hPrefab;
      inout_stream >> ia.m_Alignment;
      inout_stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        inout_stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        inout_stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        WUInt8 uiNumParams;
        inout_stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        WHashedString key;
        WVariant value;

        for (WUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          inout_stream >> key;
          inout_stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_iGroundType;
  }

  if (uiVersion >= 9)
  {
    inout_stream >> m_DebugColor;
  }
}

void WSurfaceResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 9;

  inout_stream << uiVersion;
  inout_stream << m_fPhysicsRestitution;
  inout_stream << m_fPhysicsFrictionStatic;
  inout_stream << m_fPhysicsFrictionDynamic;
  inout_stream << m_hBaseSurface;

  // version 4
  inout_stream << m_sOnCollideInteraction;

  // version 7
  inout_stream << m_sSlideInteractionPrefab;
  inout_stream << m_sRollInteractionPrefab;

  inout_stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    inout_stream << ia.m_sInteractionType;
    inout_stream << ia.m_hPrefab;
    inout_stream << ia.m_Alignment;
    inout_stream << ia.m_Deviation;

    // version 4
    inout_stream << ia.m_fImpulseThreshold;

    // version 5
    inout_stream << ia.m_fImpulseScale;

    // version 6
    const WUInt8 uiNumParams = static_cast<WUInt8>(ia.m_Parameters.GetCount());
    inout_stream << uiNumParams;
    for (WUInt32 i = 0; i < uiNumParams; ++i)
    {
      inout_stream << ia.m_Parameters.GetKey(i);
      inout_stream << ia.m_Parameters.GetValue(i);
    }
  }

  // version 8
  inout_stream << m_iGroundType;

  // version 9
  inout_stream << m_DebugColor;
}

void WSurfaceResourceDescriptor::SetCollisionInteraction(const char* szName)
{
  m_sOnCollideInteraction.Assign(szName);
}

const char* WSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void WSurfaceResourceDescriptor::SetSlideReactionPrefabFile(const char* szFile)
{
  m_sSlideInteractionPrefab.Assign(szFile);
}

const char* WSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void WSurfaceResourceDescriptor::SetRollReactionPrefabFile(const char* szFile)
{
  m_sRollInteractionPrefab.Assign(szFile);
}

const char* WSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
{
  return m_sRollInteractionPrefab.GetData();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WSurfaceResourceDescriptorPatch_1_2 : public WGraphPatch
{
public:
  WSurfaceResourceDescriptorPatch_1_2()
    : WGraphPatch("WSurfaceResourceDescriptor", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    W_IGNORE_UNUSED(ref_context);
    W_IGNORE_UNUSED(pGraph);

    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

WSurfaceResourceDescriptorPatch_1_2 g_WSurfaceResourceDescriptorPatch_1_2;


W_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);

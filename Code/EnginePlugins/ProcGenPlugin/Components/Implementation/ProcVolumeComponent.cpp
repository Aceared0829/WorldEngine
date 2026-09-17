#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WProcVolumeComponent, 1)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new WClampValueAttribute(-64.0f, 64.0f)),
    W_ENUM_ACCESSOR_PROPERTY("BlendMode", WProcGenBlendMode, GetBlendMode, SetBlendMode)->AddAttributes(new WDefaultValueAttribute(WProcGenBlendMode::Set)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnTransformChanged)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction/Procedural Generation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVolumeComponent::AreaInvalidatedEvent WProcVolumeComponent::s_AreaInvalidatedEvent;
WSpatialData::Category WProcVolumeComponent::s_SpatialCategory = WSpatialData::RegisterCategory("ProcVolume", WSpatialData::Flags::None);

WProcVolumeComponent::WProcVolumeComponent() = default;
WProcVolumeComponent::~WProcVolumeComponent() = default;

void WProcVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();

  if (GetUniqueID() != WInvalidIndex)
  {
    // Only necessary in Editor
    InvalidateArea();
  }
}

void WProcVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (GetUniqueID() != WInvalidIndex)
  {
    // Only necessary in Editor
    WBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
    if (globalBounds.IsValid())
    {
      InvalidateArea(globalBounds.GetBox());
    }
  }

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();
}

void WProcVolumeComponent::SetValue(float fValue)
{
  if (m_fValue != fValue)
  {
    m_fValue = fValue;

    InvalidateArea();
  }
}

void WProcVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = WMath::Clamp(fOrder, -64.0f, 64.0f);

  if (m_fSortOrder != fOrder)
  {
    m_fSortOrder = fOrder;

    InvalidateArea();
  }
}

void WProcVolumeComponent::SetBlendMode(WEnum<WProcGenBlendMode> blendMode)
{
  if (m_BlendMode != blendMode)
  {
    m_BlendMode = blendMode;

    InvalidateArea();
  }
}

void WProcVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fValue;
  s << m_fSortOrder;
  s << m_BlendMode;
}

void WProcVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fValue;
  s >> m_fSortOrder;
  s >> m_BlendMode;
}

void WProcVolumeComponent::OnTransformChanged(WMsgTransformChanged& ref_msg)
{
  WBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  if (!combined.IsValid())
    return;

  combined.Transform(ref_msg.m_OldGlobalTransform.GetAsMat4());

  combined.ExpandToInclude(GetOwner()->GetGlobalBounds());

  InvalidateArea(combined.GetBox());
}

void WProcVolumeComponent::InvalidateArea()
{
  if (!IsActiveAndInitialized())
    return;

  WBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void WProcVolumeComponent::InvalidateArea(const WBoundingBox& box)
{
  WProcGenInternal::InvalidatedArea area;
  area.m_Box = box;
  area.m_pWorld = GetWorld();

  s_AreaInvalidatedEvent.Broadcast(area);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WProcVolumeSphereComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractVolumes, OnExtractVolumes)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereManipulatorAttribute("Radius"),
    new WSphereVisualizerAttribute("Radius", WColorScheme::GetCategoryColor("Construction", WColorScheme::CategoryColorUsage::ViewportIcon)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVolumeSphereComponent::WProcVolumeSphereComponent() = default;
WProcVolumeSphereComponent::~WProcVolumeSphereComponent() = default;

void WProcVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void WProcVolumeSphereComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff != fFalloff)
  {
    m_fFalloff = fFalloff;

    InvalidateArea();
  }
}

void WProcVolumeSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void WProcVolumeSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;

  if (uiVersion < 2)
  {
    m_fFalloff = 1.0f - m_fFalloff;
  }
}

void WProcVolumeSphereComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), s_SpatialCategory);
}

void WProcVolumeSphereComponent::OnExtractVolumes(WMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddSphere(GetOwner()->GetGlobalTransformSimd(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFalloff);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WProcVolumeBoxComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(10.0f)), new WClampValueAttribute(WVec3(0), WVariant())),
    W_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new WDefaultValueAttribute(WVec3(0.5f)), new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f))),
    W_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new WDefaultValueAttribute(WVec3(0.5f)), new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f))),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractVolumes, OnExtractVolumes)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::GetCategoryColor("Construction", WColorScheme::CategoryColorUsage::ViewportIcon)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVolumeBoxComponent::WProcVolumeBoxComponent() = default;
WProcVolumeBoxComponent::~WProcVolumeBoxComponent() = default;

void WProcVolumeBoxComponent::SetExtents(const WVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void WProcVolumeBoxComponent::SetPositiveFalloff(const WVec3& vFalloff)
{
  if (m_vPositiveFalloff != vFalloff)
  {
    m_vPositiveFalloff = vFalloff;

    InvalidateArea();
  }
}

void WProcVolumeBoxComponent::SetNegativeFalloff(const WVec3& vFalloff)
{
  if (m_vNegativeFalloff != vFalloff)
  {
    m_vNegativeFalloff = vFalloff;

    InvalidateArea();
  }
}

void WProcVolumeBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
}

void WProcVolumeBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vPositiveFalloff;
  if (uiVersion >= 3)
  {
    s >> m_vNegativeFalloff;
  }
  else
  {
    m_vNegativeFalloff = m_vPositiveFalloff;
  }

  if (uiVersion < 2)
  {
    m_vPositiveFalloff = WVec3(1.0f) - m_vPositiveFalloff;
    m_vNegativeFalloff = WVec3(1.0f) - m_vNegativeFalloff;
  }
}

void WProcVolumeBoxComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), s_SpatialCategory);
}

void WProcVolumeBoxComponent::OnExtractVolumes(WMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vPositiveFalloff, m_vNegativeFalloff);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WProcVolumeImageComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Image", m_hImage)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_2D"), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractVolumes, OnExtractVolumes)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVolumeImageComponent::WProcVolumeImageComponent() = default;
WProcVolumeImageComponent::~WProcVolumeImageComponent() = default;

void WProcVolumeImageComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_hImage;
}

void WProcVolumeImageComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_hImage;
}

void WProcVolumeImageComponent::OnExtractVolumes(WMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vPositiveFalloff, m_vNegativeFalloff, m_hImage);
}

void WProcVolumeImageComponent::SetImage(const WImageDataResourceHandle& hResource)
{
  m_hImage = hResource;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WProcVolumeSphereComponent_1_2 : public WGraphPatch
{
public:
  WProcVolumeSphereComponent_1_2()
    : WGraphPatch("WProcVolumeSphereComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pFadeOutStart = pNode->FindProperty("FadeOutStart");
    if (pFadeOutStart && pFadeOutStart->m_Value.IsA<float>())
    {
      float fFalloff = 1.0f - pFadeOutStart->m_Value.Get<float>();
      pNode->AddProperty("Falloff", fFalloff);
    }
  }
};

WProcVolumeSphereComponent_1_2 g_WProcVolumeSphereComponent_1_2;

class WProcVolumeBoxComponent_1_2 : public WGraphPatch
{
public:
  WProcVolumeBoxComponent_1_2()
    : WGraphPatch("WProcVolumeBoxComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pFadeOutStart = pNode->FindProperty("FadeOutStart");
    if (pFadeOutStart && pFadeOutStart->m_Value.IsA<WVec3>())
    {
      WVec3 vFalloff = WVec3(1.0f) - pFadeOutStart->m_Value.Get<WVec3>();
      pNode->AddProperty("Falloff", vFalloff);
    }
  }
};

WProcVolumeBoxComponent_1_2 g_WProcVolumeBoxComponent_1_2;

class WProcVolumeBoxComponent_2_3 : public WGraphPatch
{
public:
  WProcVolumeBoxComponent_2_3()
    : WGraphPatch("WProcVolumeBoxComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pFalloff = pNode->FindProperty("Falloff");
    if (pFalloff && pFalloff->m_Value.IsA<WVec3>())
    {
      pNode->AddProperty("PositiveFalloff", pFalloff->m_Value.Get<WVec3>());
      pNode->AddProperty("NegativeFalloff", pFalloff->m_Value.Get<WVec3>());
    }
  }
};

WProcVolumeBoxComponent_2_3 g_WProcVolumeBoxComponent_2_3;


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVolumeComponent);

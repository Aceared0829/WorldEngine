#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

namespace
{
  static WVariantArray GetDefaultTags()
  {
    WVariantArray value(WStaticsAllocatorWrapper::GetAllocator());
    value.PushBack("SkyLight");
    return value;
  }
} // namespace

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSkyLightComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", WReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new WGroupAttribute("Capture Description")),
    W_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    W_ACCESSOR_PROPERTY("DiffuseIntensity", GetDiffuseIntensity, SetDiffuseIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(0.4f)),
    W_ACCESSOR_PROPERTY("DiffuseSaturation", GetDiffuseSaturation, SetDiffuseSaturation)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(0.3f)),
    W_ACCESSOR_PROPERTY("SpecularIntensity", GetSpecularIntensity, SetSpecularIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new WTagSetWidgetAttribute("Default"), new WDefaultValueAttribute(GetDefaultTags())),
    W_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, {}), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new WDefaultValueAttribute(100.0f), new WClampValueAttribute(0.01f, 10000.0f)),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnTransformChanged),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Lighting"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSkyLightComponent::WSkyLightComponent()
{
  m_Desc.m_uniqueID = WUuid::MakeUuid();
}

WSkyLightComponent::~WSkyLightComponent() = default;

void WSkyLightComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = WReflectionPool::RegisterSkyLight(GetWorld(), m_Desc, this);

  GetOwner()->UpdateLocalBounds();
}

void WSkyLightComponent::OnDeactivated()
{
  WReflectionPool::DeregisterSkyLight(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void WSkyLightComponent::SetReflectionProbeMode(WEnum<WReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

WEnum<WReflectionProbeMode> WSkyLightComponent::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

void WSkyLightComponent::SetDiffuseIntensity(float fIntensity)
{
  m_Desc.m_fDiffuseIntensity = fIntensity;
  m_bStatesDirty = true;
}

float WSkyLightComponent::GetDiffuseIntensity() const
{
  return m_Desc.m_fDiffuseIntensity;
}

void WSkyLightComponent::SetDiffuseSaturation(float fSaturation)
{
  m_Desc.m_fDiffuseSaturation = fSaturation;
  m_bStatesDirty = true;
}

float WSkyLightComponent::GetDiffuseSaturation() const
{
  return m_Desc.m_fDiffuseSaturation;
}

void WSkyLightComponent::SetSpecularIntensity(float fIntensity)
{
  m_Desc.m_fSpecularIntensity = fIntensity;
  m_bStatesDirty = true;
}

float WSkyLightComponent::GetSpecularIntensity() const
{
  return m_Desc.m_fSpecularIntensity;
}

const WTagSet& WSkyLightComponent::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void WSkyLightComponent::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void WSkyLightComponent::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

const WTagSet& WSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void WSkyLightComponent::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void WSkyLightComponent::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void WSkyLightComponent::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool WSkyLightComponent::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void WSkyLightComponent::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool WSkyLightComponent::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void WSkyLightComponent::SetCubeMapFile(WStringView sFile)
{
  WTextureCubeResourceHandle hCubeMap;

  if (!sFile.IsEmpty())
  {
    hCubeMap = WResourceManager::LoadResource<WTextureCubeResource>(sFile);
  }

  m_hCubeMap = hCubeMap;
  m_bStatesDirty = true;
}

WStringView WSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.GetResourceID();
}

void WSkyLightComponent::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void WSkyLightComponent::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void WSkyLightComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic);
}

void WSkyLightComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    WReflectionPool::UpdateSkyLight(GetWorld(), m_Id, m_Desc, this);
  }

  WReflectionPool::ExtractReflectionProbe(this, msg, nullptr, GetWorld(), m_Id, WMath::MaxValue<float>());
}

void WSkyLightComponent::OnTransformChanged(WMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void WSkyLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_fDiffuseIntensity;
  s << m_Desc.m_fDiffuseSaturation;
  s << m_Desc.m_fSpecularIntensity;
  s << m_hCubeMap;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
}

void WSkyLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_fDiffuseIntensity;
  s >> m_Desc.m_fDiffuseSaturation;
  if (uiVersion >= 4)
  {
    s >> m_Desc.m_fSpecularIntensity;
  }

  if (uiVersion >= 2)
  {
    s >> m_hCubeMap;
  }
  if (uiVersion >= 3)
  {
    s >> m_Desc.m_fNearPlane;
    s >> m_Desc.m_fFarPlane;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WSkyLightComponentPatch_2_3 : public WGraphPatch
{
public:
  WSkyLightComponentPatch_2_3()
    : WGraphPatch("WSkyLightComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Inline ReflectionData sub-object into the sky light itself.
    if (const WAbstractObjectNode::Property* pProp0 = pNode->FindProperty("ReflectionData"))
    {
      if (pProp0->m_Value.IsA<WUuid>())
      {
        if (WAbstractObjectNode* pSubNode = pGraph->GetNode(pProp0->m_Value.Get<WUuid>()))
        {
          for (auto pProp : pSubNode->GetProperties())
          {
            pNode->AddProperty(pProp.m_sPropertyName, pProp.m_Value);
          }
        }
      }
    }
  }
};

WSkyLightComponentPatch_2_3 g_WSkyLightComponentPatch_2_3;

///////////////////////////////////////////////////////////////////////////

class WSkyLightComponentPatch_3_4 : public WGraphPatch
{
public:
  WSkyLightComponentPatch_3_4()
    : WGraphPatch("WSkyLightComponent", 4)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Intensity", "DiffuseIntensity");
    pNode->RenameProperty("Saturation", "DiffuseSaturation");
  }
};

WSkyLightComponentPatch_3_4 g_WSkyLightComponentPatch_3_4;

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);

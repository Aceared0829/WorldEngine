#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
extern WCVarBool cvar_RenderingLightingVisScreenSpaceSize;
#endif

constexpr WAngle c_MaxSpotAngle = WAngle::MakeFromDegree(160.0f);

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpotLightRenderData, 1, WRTTIDefaultAllocator<WSpotLightRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WSpotLightComponent, 5, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(0.0f), new WSuffixAttribute(" m"), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WClampValueAttribute(0.0f, 0.5f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeZero(), c_MaxSpotAngle), new WDefaultValueAttribute(WAngle::MakeFromDegree(15.0f))),
    W_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeZero(), c_MaxSpotAngle), new WDefaultValueAttribute(WAngle::MakeFromDegree(30.0f))),
    W_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WSuffixAttribute(" m"), new WMinValueTextAttribute("Auto")),
    W_RESOURCE_ACCESSOR_PROPERTY("Cookie", GetCookie, SetCookie)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    W_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Decal")),
    W_ACCESSOR_PROPERTY("MaterialResolution", GetMaterialResolution, SetMaterialResolution)->AddAttributes(new WClampValueAttribute(16, 1024), new WDefaultValueAttribute(512)),
    W_ACCESSOR_PROPERTY("MaterialUpdateInterval", GetMaterialUpdateInterval, SetMaterialUpdateInterval)->AddAttributes(new WClampValueAttribute(0.0, 10.0), new WDefaultValueAttribute(0.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor", "Radius"),
    new WConeLengthManipulatorAttribute("Range"),
    new WConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new WConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSpotLightComponent::WSpotLightComponent() = default;
WSpotLightComponent::~WSpotLightComponent() = default;

void WSpotLightComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateCookie();
}

void WSpotLightComponent::OnDeactivated()
{
  DeleteCookie();

  SUPER::OnDeactivated();
}

void WSpotLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_fShadowFadeOutRange;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
  s << m_uiMaterialResolution;
  s << m_MaterialUpdateInterval;
  s << m_hMaterial;
  s << m_hCookie;
  s << m_fRadius;
}

void WSpotLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  WTexture2DResourceHandle m_hProjectedTexture;

  s >> m_fRange;
  if (uiVersion >= 3)
  {
    s >> m_fShadowFadeOutRange;
  }
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  if (uiVersion >= 4)
  {
    s >> m_uiMaterialResolution;
    s >> m_MaterialUpdateInterval;
    s >> m_hMaterial;
    s >> m_hCookie;
  }
  else
  {
    WStringBuilder temp;
    s >> temp;
    SetCookieFile(temp);
  }

  if (uiVersion >= 5)
  {
    s >> m_fRadius;
  }
}

WResult WSpotLightComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = CalculateBoundingSphere(WTransform::MakeIdentity(), m_fEffectiveRange);
  return W_SUCCESS;
}

void WSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float WSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float WSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void WSpotLightComponent::SetRadius(float fRadius)
{
  m_fRadius = WMath::Max(fRadius, 0.0f);

  InvalidateCachedRenderData();
}

float WSpotLightComponent::GetRadius() const
{
  return m_fRadius;
}

void WSpotLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = WMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float WSpotLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void WSpotLightComponent::SetInnerSpotAngle(WAngle spotAngle)
{
  m_InnerSpotAngle = WMath::Clamp(spotAngle, WAngle::MakeZero(), c_MaxSpotAngle);

  InvalidateCachedRenderData();
}

WAngle WSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void WSpotLightComponent::SetOuterSpotAngle(WAngle spotAngle)
{
  m_OuterSpotAngle = WMath::Clamp(spotAngle, WAngle::MakeZero(), c_MaxSpotAngle);

  TriggerLocalBoundsUpdate();
}

WAngle WSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void WSpotLightComponent::SetCookie(const WTexture2DResourceHandle& hCookie)
{
  if (m_hCookie != hCookie)
  {
    m_hCookie = hCookie;

    UpdateCookie();
    InvalidateCachedRenderData();
  }
}

void WSpotLightComponent::SetMaterial(const WMaterialResourceHandle& hMaterial)
{
  if (m_hMaterial != hMaterial)
  {
    m_hMaterial = hMaterial;

    UpdateCookie();
    InvalidateCachedRenderData();
  }
}

void WSpotLightComponent::SetMaterialResolution(WUInt32 uiResolution)
{
  m_uiMaterialResolution = WMath::Clamp(uiResolution, 16u, 1024u);

  UpdateCookie();
  // No need to invalidate cached render data, render data is not cached if a material is used.
}

void WSpotLightComponent::SetMaterialUpdateInterval(WTime updateInterval)
{
  m_MaterialUpdateInterval = WMath::Clamp(updateInterval.AsFloatInSeconds(), 0.0f, 10.0f);

  UpdateCookie();
  // No need to invalidate cached render data, render data is not cached if a material is used.
}

void WSpotLightComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  const WTransform t = GetOwner()->GetGlobalTransform();
  WBoundingSphere bounds = CalculateBoundingSphere(t, m_fEffectiveRange);
  bounds.m_vCenter = (bounds.m_vCenter + t.m_vPosition) * 0.5f; // Halfway between light origin and cone center

  const float fScreenSpaceSize = CalculateScreenSpaceSize(bounds, *msg.m_pView->GetCullingCamera());
  float fShadowScreenSize = 0.0f;
  const float fShadowFadeOut = CalculateShadowFadeOut(bounds, m_fShadowFadeOutRange, *msg.m_pView->GetCullingCamera(), fShadowScreenSize);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  VisualizeScreenSpaceSize(msg.m_pView->GetHandle(), bounds, fScreenSpaceSize, fShadowScreenSize, fShadowFadeOut);
#endif

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WSpotLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;

  pRenderData->m_qGlobalRotation = t.m_qRotation;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_fRadius = m_fRadius;
  pRenderData->m_InnerSpotAngle = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle = m_OuterSpotAngle;
  pRenderData->m_CookieId = m_CookieId;

  if (m_CookieId.IsInvalidated() == false)
  {
    // Spotlight bounds tend to be way larger than the projected area thus times 0.5
    const float fScreenSpaceSizeForCookie = fScreenSpaceSize * 0.5f;

    WDecalManager::MarkRuntimeDecalAsUsed(m_CookieId, fScreenSpaceSizeForCookie, msg.m_pView);
  }

  if (m_bCastShadows && fShadowFadeOut > 0.0f)
  {
    pRenderData->FillShadowDataOffsetAndFadeOut(WShadowPool::AddSpotLight(this, fScreenSpaceSize, msg.m_pView), fShadowFadeOut);
  }
  else
  {
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
  }

  pRenderData->FillSortingKey(fScreenSpaceSize);

  WRenderData::Caching::Enum caching = (m_bCastShadows || m_CookieId.IsInvalidated() == false) ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
    caching = WRenderData::Caching::Never;
#endif
  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, caching);
}

WBoundingSphere WSpotLightComponent::CalculateBoundingSphere(const WTransform& t, float fRange) const
{
  WBoundingSphere res;
  WAngle halfAngle = m_OuterSpotAngle / 2.0f;
  WVec3 position = t.m_vPosition;
  WVec3 forwardDir = t.m_qRotation * WVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > WAngle::MakeFromDegree(45.0f))
  {
    res.m_vCenter = position + WMath::Cos(halfAngle) * fRange * forwardDir;
    res.m_fRadius = WMath::Sin(halfAngle) * fRange;
  }
  else
  {
    res.m_fRadius = fRange / (2.0f * WMath::Cos(halfAngle));
    res.m_vCenter = position + forwardDir * res.m_fRadius;
  }

  return res;
}

void WSpotLightComponent::UpdateCookie()
{
  if (!IsActiveAndInitialized())
    return;

  DeleteCookie();

  if (m_hMaterial.IsValid())
  {
    m_CookieId = WDecalManager::GetOrCreateRuntimeDecal(m_hMaterial, m_uiMaterialResolution, WTime::MakeFromSeconds(m_MaterialUpdateInterval));
  }
  else if (m_hCookie.IsValid())
  {
    m_CookieId = WDecalManager::GetOrCreateRuntimeDecal(m_hCookie);
  }
}

void WSpotLightComponent::DeleteCookie()
{
  WDecalManager::DeleteRuntimeDecal(m_CookieId);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpotLightVisualizerAttribute, 1, WRTTIDefaultAllocator<WSpotLightVisualizerAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSpotLightVisualizerAttribute::WSpotLightVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WSpotLightVisualizerAttribute::WSpotLightVisualizerAttribute(
  const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty, const char* szRadiusProperty)
  : WVisualizerAttribute(szAngleProperty, szRangeProperty, szIntensityProperty, szColorProperty, szRadiusProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSpotLightComponentPatch_1_2 : public WGraphPatch
{
public:
  WSpotLightComponentPatch_1_2()
    : WGraphPatch("WSpotLightComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("WLightComponent", 2, true);

    pNode->RenameProperty("Inner Spot Angle", "InnerSpotAngle");
    pNode->RenameProperty("Outer Spot Angle", "OuterSpotAngle");
  }
};

WSpotLightComponentPatch_1_2 g_WSpotLightComponentPatch_1_2;


W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);

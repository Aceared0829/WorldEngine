#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LightShaftsComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLightShaftsRenderData, 1, WRTTIDefaultAllocator<WLightShaftsRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WLightShaftsComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_ACCESSOR_PROPERTY("BrightnessThreshold", GetBrightnessThreshold, SetBrightnessThreshold)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(0.0f)),
    W_ACCESSOR_PROPERTY("MaxBrightness", GetMaxBrightness, SetMaxBrightness)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(10.0f)),
    W_ACCESSOR_PROPERTY("DiskMaskRadius", GetDiskMaskRadius, SetDiskMaskRadius)->AddAttributes(new WClampValueAttribute(0.0f, 2.0f), new WDefaultValueAttribute(0.1f)),
    W_ACCESSOR_PROPERTY("TintColor", GetTintColor, SetTintColor),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WLightShaftsComponent::WLightShaftsComponent() = default;
WLightShaftsComponent::~WLightShaftsComponent() = default;

void WLightShaftsComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_fIntensity;
  s << m_fMaxBrightness;
  s << m_fBrightnessThreshold;
  s << m_fDiskMaskRadius;
  s << m_TintColor;
}

void WLightShaftsComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_fIntensity;
  s >> m_fMaxBrightness;
  s >> m_fBrightnessThreshold;
  s >> m_fDiskMaskRadius;

  if (uiVersion >= 2)
  {
    s >> m_TintColor;
  }
}

WResult WLightShaftsComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return W_SUCCESS;
}

void WLightShaftsComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = WMath::Max(fIntensity, 0.0f);

  if (IsActiveAndInitialized())
  {
    InvalidateCachedRenderData();
  }
}

void WLightShaftsComponent::SetBrightnessThreshold(float fBrightnessThreshold)
{
  m_fBrightnessThreshold = WMath::Max(fBrightnessThreshold, 0.0f);

  if (IsActiveAndInitialized())
  {
    InvalidateCachedRenderData();
  }
}

void WLightShaftsComponent::SetMaxBrightness(float fMaxBrightness)
{
  m_fMaxBrightness = WMath::Max(fMaxBrightness, 0.0f);

  if (IsActiveAndInitialized())
  {
    InvalidateCachedRenderData();
  }
}

void WLightShaftsComponent::SetDiskMaskRadius(float fDiskMaskRadius)
{
  m_fDiskMaskRadius = WMath::Clamp(fDiskMaskRadius, 0.0f, 2.0f);

  if (IsActiveAndInitialized())
  {
    InvalidateCachedRenderData();
  }
}

void WLightShaftsComponent::SetTintColor(const WColorGammaUB& color)
{
  m_TintColor = color;

  if (IsActiveAndInitialized())
  {
    InvalidateCachedRenderData();
  }
}

void WLightShaftsComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't render in shadow and reflection views
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Reflection)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WLightShaftsRenderData>(GetOwner());

  pRenderData->m_vDirection = GetOwner()->GetGlobalRotation() * WVec3(-1, 0, 0);
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fMaxBrightness = m_fMaxBrightness;
  pRenderData->m_fBrightnessThreshold = m_fBrightnessThreshold;
  pRenderData->m_fDiskMaskRadius = m_fDiskMaskRadius;
  pRenderData->m_TintColor = m_TintColor;

  pRenderData->m_uiSortingKey = WInvalidIndex;

  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, WRenderData::Caching::IfStatic);
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LightShaftsComponent);

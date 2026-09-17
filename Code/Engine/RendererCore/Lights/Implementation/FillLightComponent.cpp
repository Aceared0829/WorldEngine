#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/FillLightComponent.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
extern WCVarBool cvar_RenderingLightingVisScreenSpaceSize;
#endif

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WFillLightMode, 1)
  W_ENUM_CONSTANTS(WFillLightMode::Additive, WFillLightMode::Subtractive, WFillLightMode::ModulateIndirect)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFillLightRenderData, 1, WRTTIDefaultAllocator<WFillLightRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

void WFillLightRenderData::FillSortingKey(float fScreenSpaceSize)
{
  const WUInt32 uiSortingKey = 10000u - static_cast<WUInt32>(WMath::Clamp(fScreenSpaceSize, 0.0f, 10.0f) * 1000.0f);
  m_uiSortingKey = 0x1000000 + uiSortingKey;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_COMPONENT_TYPE(WFillLightComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY_READ_ONLY("EffectiveColor", GetEffectiveColor)->AddAttributes(new WHiddenAttribute),
    W_ENUM_ACCESSOR_PROPERTY("LightMode", WFillLightMode, GetLightMode, SetLightMode),
    W_ACCESSOR_PROPERTY("UseColorTemperature", GetUsingColorTemperature, SetUsingColorTemperature),
    W_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    W_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new WImageSliderUiAttribute("LightTemperature"), new WDefaultValueAttribute(6550), new WClampValueAttribute(1000, 15000)),
    W_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(5.0f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("FalloffExponent", GetFalloffExponent, SetFalloffExponent)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_ACCESSOR_PROPERTY("Directionality", GetDirectionality, SetDirectionality)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f), new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Lighting"),
    new WSphereManipulatorAttribute("Range"),
    new WSphereVisualizerAttribute("Range", WColor::White, "LightColor"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WFillLightComponent::WFillLightComponent() = default;
WFillLightComponent::~WFillLightComponent() = default;

void WFillLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_uiTemperature;
  s << m_fIntensity;
  s << m_fRange;
  s << m_fFalloffExponent;
  s << m_fDirectionality;
  s << m_LightMode;
  s << m_bUseColorTemperature;
}

void WFillLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_uiTemperature;
  s >> m_fIntensity;
  s >> m_fRange;
  s >> m_fFalloffExponent;
  s >> m_fDirectionality;
  s >> m_LightMode;
  s >> m_bUseColorTemperature;
}

WResult WFillLightComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRange);
  return W_SUCCESS;
}

void WFillLightComponent::SetLightMode(WEnum<WFillLightMode> mode)
{
  m_LightMode = mode;

  InvalidateCachedRenderData();
}

void WFillLightComponent::SetUsingColorTemperature(bool bUseColorTemperature)
{
  m_bUseColorTemperature = bUseColorTemperature;

  InvalidateCachedRenderData();
}

void WFillLightComponent::SetTemperature(WUInt32 uiTemperature)
{
  m_uiTemperature = WMath::Clamp(uiTemperature, 1500u, 40000u);

  InvalidateCachedRenderData();
}

void WFillLightComponent::SetLightColor(WColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

WColorGammaUB WFillLightComponent::GetEffectiveColor() const
{
  if (m_bUseColorTemperature)
  {
    return WColor::MakeFromKelvin(m_uiTemperature);
  }
  else
  {
    return m_LightColor;
  }
}

void WFillLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  InvalidateCachedRenderData();
}

void WFillLightComponent::SetRange(float fRange)
{
  m_fRange = WMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

void WFillLightComponent::SetFalloffExponent(float fFalloffExponent)
{
  m_fFalloffExponent = WMath::Max(fFalloffExponent, 0.0f);

  InvalidateCachedRenderData();
}

void WFillLightComponent::SetDirectionality(float fDirectionality)
{
  m_fDirectionality = WMath::Saturate(fDirectionality);

  InvalidateCachedRenderData();
}

void WFillLightComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

void WFillLightComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if ((m_LightMode == WFillLightMode::Additive && WMath::IsZero(m_fIntensity, WMath::DefaultEpsilon<float>())) || m_fRange <= 0.0f)
    return;

  const WTransform t = GetOwner()->GetGlobalTransform();
  const WBoundingSphere bs = WBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fRange);

  const float fScreenSpaceSize = WLightComponent::CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    WColor c = WColorScheme::LightUI(WColorScheme::Cyan);
    WDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), WFmt("{0}", fScreenSpaceSize), t.m_vPosition, c);
    WDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, c);
  }
#endif

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WFillLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_LightMode = m_LightMode;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
  pRenderData->m_fFalloffExponent = m_fFalloffExponent;
  pRenderData->m_fDirectionality = m_fDirectionality;

  pRenderData->FillSortingKey(fScreenSpaceSize);

  WRenderData::Caching::Enum caching = WRenderData::Caching::IfStatic;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
    caching = WRenderData::Caching::Never;
#endif
  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, caching);
}


W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_FillLightComponent);

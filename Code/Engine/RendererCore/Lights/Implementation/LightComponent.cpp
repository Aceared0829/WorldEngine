#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/LightComponent.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_RenderingLightingVisScreenSpaceSize("Rendering.Lighting.VisScreenSpaceSize", false, WCVarFlags::Default, "Enables debug visualization of light screen space size calculation");
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLightRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WLightRenderData::CanBatch(const WRenderData& other) const
{
  return true;
}

void WLightRenderData::FillSortingKey(float fScreenSpaceSize)
{
  const float fMultiplier = (m_uiShadowDataOffsetAndFadeOut != 0) ? 1000.0f : 500.0f;
  const WUInt32 uiSortingKey = 10000u - static_cast<WUInt32>(WMath::Clamp(fScreenSpaceSize, 0.0f, 10.0f) * fMultiplier);
  m_uiSortingKey = s_uiBaseSortingKey + uiSortingKey;
}

void WLightRenderData::FillShadowDataOffsetAndFadeOut(WUInt32 uiDataOffset, float fFadeOut)
{
  WUInt32 uiFadeOut = WMath::ColorFloatToUnsignedInt<12>(fFadeOut);
  m_uiShadowDataOffsetAndFadeOut = uiDataOffset | (uiFadeOut << 20);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WLightComponent, 6)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY_READ_ONLY("EffectiveColor", GetEffectiveColor)->AddAttributes(new WHiddenAttribute),
    W_ACCESSOR_PROPERTY("UseColorTemperature", GetUsingColorTemperature, SetUsingColorTemperature),
    W_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    W_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new WImageSliderUiAttribute("LightTemperature"), new WDefaultValueAttribute(6550), new WClampValueAttribute(1000, 15000)),
    W_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(10.0f)),
    W_ACCESSOR_PROPERTY("SpecularMultiplier", GetSpecularMultiplier, SetSpecularMultiplier)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    W_ACCESSOR_PROPERTY("TransparentShadows", GetTransparentShadows, SetTransparentShadows),
    W_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new WClampValueAttribute(0.0f, 0.5f), new WDefaultValueAttribute(0.05f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f), new WDefaultValueAttribute(0.25f)),
    W_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f), new WDefaultValueAttribute(0.1f))
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Lighting"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WLightComponent::WLightComponent() = default;
WLightComponent::~WLightComponent() = default;

void WLightComponent::SetUsingColorTemperature(bool bUseColorTemperature)
{
  if (m_bUseColorTemperature != bUseColorTemperature)
  {
    m_bUseColorTemperature = bUseColorTemperature;

    InvalidateCachedRenderData();
  }
}

bool WLightComponent::GetUsingColorTemperature() const
{
  return m_bUseColorTemperature;
}

void WLightComponent::SetTemperature(WUInt32 uiTemperature)
{
  uiTemperature = WMath::Clamp(uiTemperature, 1500u, 40000u);

  if (m_uiTemperature != uiTemperature)
  {
    m_uiTemperature = uiTemperature;

    InvalidateCachedRenderData();
  }
}

WUInt32 WLightComponent::GetTemperature() const
{
  return m_uiTemperature;
}

void WLightComponent::SetLightColor(WColorGammaUB lightColor)
{
  if (m_LightColor != lightColor)
  {
    m_LightColor = lightColor;

    InvalidateCachedRenderData();
  }
}

WColorGammaUB WLightComponent::GetLightColor() const
{
  return m_LightColor;
}

WColorGammaUB WLightComponent::GetEffectiveColor() const
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

void WLightComponent::SetIntensity(float fIntensity)
{
  fIntensity = WMath::Max(fIntensity, 0.0f);

  if (m_fIntensity != fIntensity)
  {
    m_fIntensity = fIntensity;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

float WLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void WLightComponent::SetSpecularMultiplier(float fSpecularMultiplier)
{
  fSpecularMultiplier = WMath::Max(fSpecularMultiplier, 0.0f);

  if (m_fSpecularMultiplier != fSpecularMultiplier)
  {
    m_fSpecularMultiplier = fSpecularMultiplier;

    InvalidateCachedRenderData();
  }
}

float WLightComponent::GetSpecularMultiplier() const
{
  return m_fSpecularMultiplier;
}

void WLightComponent::SetCastShadows(bool bCastShadows)
{
  if (m_bCastShadows != bCastShadows)
  {
    m_bCastShadows = bCastShadows;

    InvalidateCachedRenderData();
  }
}

bool WLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void WLightComponent::SetTransparentShadows(bool bShadows)
{
  if (m_bTransparentShadows != bShadows)
  {
    m_bTransparentShadows = bShadows;

    InvalidateCachedRenderData();
  }
}

bool WLightComponent::GetTransparentShadows() const
{
  return m_bTransparentShadows;
}

void WLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  if (m_fPenumbraSize != fPenumbraSize)
  {
    m_fPenumbraSize = fPenumbraSize;

    InvalidateCachedRenderData();
  }
}

float WLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void WLightComponent::SetSlopeBias(float fBias)
{
  if (m_fSlopeBias != fBias)
  {
    m_fSlopeBias = fBias;

    InvalidateCachedRenderData();
  }
}

float WLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void WLightComponent::SetConstantBias(float fBias)
{
  if (m_fConstantBias != fBias)
  {
    m_fConstantBias = fBias;

    InvalidateCachedRenderData();
  }
}

float WLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void WLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
  s << m_bTransparentShadows;
  s << m_bUseColorTemperature;
  s << m_uiTemperature;
  s << m_fSpecularMultiplier;
}

void WLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

  if (uiVersion >= 3)
  {
    s >> m_fPenumbraSize;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSlopeBias;
    s >> m_fConstantBias;
  }

  s >> m_bCastShadows;

  if (uiVersion >= 6)
  {
    s >> m_bTransparentShadows;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseColorTemperature;
    s >> m_uiTemperature;
    s >> m_fSpecularMultiplier;
  }
}

void WLightComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  WColor newColor = m_LightColor;
  ref_msg.ModifyColor(newColor);

  if (m_LightColor != newColor)
  {
    m_LightColor = newColor;

    InvalidateCachedRenderData();
  }
}

// static
float WLightComponent::CalculateEffectiveRange(float fRange, float fIntensity)
{
  const float fThreshold = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = WMath::Sqrt(WMath::Max(0.0f, fIntensity)) / WMath::Sqrt(fThreshold);

  W_ASSERT_DEBUG(!WMath::IsNaN(fEffectiveRange), "Light range is NaN");

  if (fRange <= 0.0f)
  {
    return fEffectiveRange;
  }

  return WMath::Min(fRange, fEffectiveRange);
}

// static
float WLightComponent::CalculateScreenSpaceSize(const WBoundingSphere& sphere, const WCamera& camera)
{
  if (camera.IsPerspective())
  {
    float dist = (sphere.m_vCenter - camera.GetPosition()).GetLength();
    float fHalfHeight = WMath::Tan(camera.GetFovY(1.0f) * 0.5f) * dist;
    return sphere.m_fRadius / fHalfHeight;
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

float WLightComponent::CalculateShadowFadeOut(const WBoundingSphere& sphere, float fShadowFadeOutRange, const WCamera& camera, float& out_fShadowScreenSize) const
{
  if (!m_bCastShadows)
    return 0.0f;

  WBoundingSphere shadowBounds = sphere;
  if (fShadowFadeOutRange > 0.0f)
  {
    shadowBounds.m_fRadius = fShadowFadeOutRange;
  }
  out_fShadowScreenSize = CalculateScreenSpaceSize(shadowBounds, camera);
  return WMath::Saturate(WMath::Unlerp(0.8f, 1.0f, out_fShadowScreenSize));
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
void WLightComponent::VisualizeScreenSpaceSize(WViewHandle hView, const WBoundingSphere& sphere, float fScreenSize, float fShadowScreenSize, float fShadowFadeOut) const
{
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    WColor c = WColorScheme::LightUI(WColorScheme::Cyan);
    if (m_bCastShadows)
    {
      WDebugRenderer::Draw3DText(hView,
        WFmt("ScreenSize: {}\nShadowScreenSize: {}\n ShadowFadeOut: {}", WArgF(fScreenSize, 3), WArgF(fShadowScreenSize, 3), WArgF(fShadowFadeOut, 3)), sphere.m_vCenter, c);
    }
    else
    {
      WDebugRenderer::Draw3DText(hView, WFmt("ScreenSize: {}", WArgF(fScreenSize, 3)), sphere.m_vCenter, c);
    }
    WDebugRenderer::DrawLineSphere(hView, sphere, c);
  }
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WLightComponentPatch_1_2 : public WGraphPatch
{
public:
  WLightComponentPatch_1_2()
    : WGraphPatch("WLightComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override { pNode->RenameProperty("Light Color", "LightColor"); }
};

WLightComponentPatch_1_2 g_WLightComponentPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);

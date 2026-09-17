#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
extern WCVarBool cvar_RenderingLightingVisScreenSpaceSize;
#endif

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPointLightRenderData, 1, WRTTIDefaultAllocator<WPointLightRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WPointLightComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WClampValueAttribute(0.0f, 0.5f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WSuffixAttribute(" m"), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WSuffixAttribute(" m"), new WMinValueTextAttribute("Auto")),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereManipulatorAttribute("Range"),
    new WPointLightVisualizerAttribute("Length", "Radius", "Range", "Intensity", "LightColor"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WPointLightComponent::WPointLightComponent() = default;
WPointLightComponent::~WPointLightComponent() = default;

WResult WPointLightComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  const float fBoundingRadius = m_fEffectiveRange + m_fLength * 0.5f;
  ref_bounds = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), fBoundingRadius);
  return W_SUCCESS;
}

void WPointLightComponent::SetRange(float fRange)
{
  m_fRange = WMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

float WPointLightComponent::GetRange() const
{
  return m_fRange;
}

float WPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void WPointLightComponent::SetLength(float fLength)
{
  m_fLength = WMath::Max(fLength, 0.0f);
  TriggerLocalBoundsUpdate();
}

float WPointLightComponent::GetLength() const
{
  return m_fLength;
}

void WPointLightComponent::SetRadius(float fRadius)
{
  m_fRadius = WMath::Max(fRadius, 0.0f);
  InvalidateCachedRenderData();
}

float WPointLightComponent::GetRadius() const
{
  return m_fRadius;
}

void WPointLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = WMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float WPointLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void WPointLightComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  const WTransform t = GetOwner()->GetGlobalTransform();
  const bool bIsTubeLight = (m_fLength > 0.0f || m_fRadius > 0.0f);
  // Clamp to minimum to avoid degenerate TubeLightShading shader math (division by zero when halfLength=0)
  const float fEffectiveLength = bIsTubeLight ? WMath::Max(m_fLength, 0.001f) : 0.0f;
  const float fEffectiveRadius = bIsTubeLight ? WMath::Max(m_fRadius, 0.001f) : 0.0f;
  const float fBoundingRadius = m_fEffectiveRange + fEffectiveLength * 0.5f;

  const WBoundingSphere bounds = WBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, fBoundingRadius);
  const float fScreenSpaceSize = CalculateScreenSpaceSize(bounds, *msg.m_pView->GetCullingCamera());
  float fShadowScreenSize = 0.0f;
  const float fShadowFadeOut = CalculateShadowFadeOut(bounds, m_fShadowFadeOutRange, *msg.m_pView->GetCullingCamera(), fShadowScreenSize);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  VisualizeScreenSpaceSize(msg.m_pView->GetHandle(), bounds, fScreenSpaceSize, fShadowScreenSize, fShadowFadeOut);
#endif

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WPointLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_qGlobalRotation = t.m_qRotation;
  pRenderData->m_fLength = fEffectiveLength;
  pRenderData->m_fRadius = fEffectiveRadius;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    VisualizeScreenSpaceSize(msg.m_pView->GetHandle(), bounds, fScreenSpaceSize, fShadowScreenSize, fShadowFadeOut);

    WMat4 capsuleMat = WMat4::MakeTranslation(t.m_vPosition);
    WMat3 rotXtoZ;
    rotXtoZ.SetColumn(0, WVec3(0, 0, 1));
    rotXtoZ.SetColumn(1, WVec3(0, 1, 0));
    rotXtoZ.SetColumn(2, WVec3(-1, 0, 0));
    capsuleMat.SetRotationalPart(t.m_qRotation.GetAsMat3() * rotXtoZ);
    WDebugRenderer::DrawLineCapsuleZ(msg.m_pView->GetHandle(), fEffectiveLength, fEffectiveRadius, WColorScheme::LightUI(WColorScheme::Yellow), capsuleMat);
  }
#endif

  if (m_bCastShadows && fShadowFadeOut > 0.0f)
  {
    pRenderData->FillShadowDataOffsetAndFadeOut(WShadowPool::AddPointLight(this, fScreenSpaceSize, msg.m_pView), fShadowFadeOut);
  }
  else
  {
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
  }

  pRenderData->FillSortingKey(fScreenSpaceSize);

  WRenderData::Caching::Enum caching = m_bCastShadows ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
    caching = WRenderData::Caching::Never;
#endif
  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, caching);
}

void WPointLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  WTextureCubeResourceHandle m_hProjectedTexture;

  s << m_fRange;
  s << m_fShadowFadeOutRange;
  s << m_hProjectedTexture;
  s << m_fLength;
  s << m_fRadius;
}

void WPointLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  WTextureCubeResourceHandle m_hProjectedTexture;

  s >> m_fRange;
  if (uiVersion >= 3)
  {
    s >> m_fShadowFadeOutRange;
  }
  s >> m_hProjectedTexture;
  if (uiVersion >= 4)
  {
    s >> m_fLength;
    s >> m_fRadius;
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPointLightVisualizerAttribute, 1, WRTTIDefaultAllocator<WPointLightVisualizerAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

WPointLightVisualizerAttribute::WPointLightVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WPointLightVisualizerAttribute::WPointLightVisualizerAttribute(
  const char* szLengthProperty, const char* szRadiusProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : WVisualizerAttribute(szLengthProperty, szRadiusProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WPointLightComponentPatch_1_2 : public WGraphPatch
{
public:
  WPointLightComponentPatch_1_2()
    : WGraphPatch("WPointLightComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("WLightComponent", 2, true);
  }
};

WPointLightComponentPatch_1_2 g_WPointLightComponentPatch_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);

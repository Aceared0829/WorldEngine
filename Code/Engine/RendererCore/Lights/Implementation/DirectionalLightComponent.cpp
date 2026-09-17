#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDirectionalLightRenderData, 1, WRTTIDefaultAllocator<WDirectionalLightRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WDirectionalLightComponent, 5, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("SourceAngle", GetSourceAngle, SetSourceAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeZero(), WAngle::MakeFromDegree(10.0f))),
    W_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new WClampValueAttribute(1, 4), new WDefaultValueAttribute(2)),
    W_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new WClampValueAttribute(0.1f, WVariant()), new WDefaultValueAttribute(30.0f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new WClampValueAttribute(0.6f, 1.0f), new WDefaultValueAttribute(0.8f)),
    W_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f), new WDefaultValueAttribute(0.7f)),
    W_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(100.0f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("ScreenSpaceShadows", GetScreenSpaceShadows, SetScreenSpaceShadows),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 1.0f, WColor::White, "LightColor"),
    new WDirectionalLightVisualizerAttribute("SourceAngle", "LightColor"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WDirectionalLightComponent::WDirectionalLightComponent() = default;
WDirectionalLightComponent::~WDirectionalLightComponent() = default;

WResult WDirectionalLightComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return W_SUCCESS;
}

void WDirectionalLightComponent::SetScreenSpaceShadows(bool bShadows)
{
  m_bScreenSpaceShadows = bShadows;

  InvalidateCachedRenderData();
}

bool WDirectionalLightComponent::GetScreenSpaceShadows() const
{
  return m_bScreenSpaceShadows;
}

void WDirectionalLightComponent::SetSourceAngle(WAngle sourceAngle)
{
  m_SourceAngle = WMath::Clamp(sourceAngle, WAngle::MakeZero(), WAngle::MakeFromDegree(10.0f));

  InvalidateCachedRenderData();
}

WAngle WDirectionalLightComponent::GetSourceAngle() const
{
  return m_SourceAngle;
}

void WDirectionalLightComponent::SetNumCascades(WUInt32 uiNumCascades)
{
  m_uiNumCascades = WMath::Clamp(uiNumCascades, 1u, 4u);

  InvalidateCachedRenderData();
}

WUInt32 WDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void WDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = WMath::Max(fMinShadowRange, 0.0f);

  InvalidateCachedRenderData();
}

float WDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void WDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = WMath::Clamp(fFadeOutStart, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float WDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void WDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = WMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float WDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void WDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = WMath::Max(fNearPlaneOffset, 0.0f);

  InvalidateCachedRenderData();
}

float WDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void WDirectionalLightComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WDirectionalLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;

  pRenderData->m_vDirection = GetOwner()->GetGlobalRotation() * WVec3(-1, 0, 0);
  // The shader reinterprets WLightRenderData::m_fRadius for directional lights as sin(halfAngle)
  // of the emitter disc. This keeps the packed fp16 slot (specularMultiplierAndRadius) uniform
  // across all light types while giving directional lights an angular parameterisation.
  pRenderData->m_fRadius = WMath::Sin(m_SourceAngle * 0.5f);
  pRenderData->m_bScreenSpaceShadows = m_bScreenSpaceShadows;

  if (m_bCastShadows)
  {
    pRenderData->FillShadowDataOffsetAndFadeOut(WShadowPool::AddDirectionalLight(this, msg.m_pView), 1.0f);
  }
  else
  {
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
  }

  // Sorting key
  {
    const float fShadowMultiplier = m_bCastShadows ? 1.0f : 0.5f;
    const float fIntensity = (m_fIntensity * WColor(pRenderData->m_LightColor).GetLuminance() * fShadowMultiplier);
    pRenderData->m_uiSortingKey = pRenderData->s_uiBaseSortingKey - WMath::Clamp(static_cast<WUInt32>(fIntensity), 0u, pRenderData->s_uiBaseSortingKey - 1);
  }

  WRenderData::Caching::Enum caching = m_bCastShadows ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, caching);
}

void WDirectionalLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_bScreenSpaceShadows;
  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
  s << m_SourceAngle;
}

void WDirectionalLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  if (uiVersion >= 4)
  {
    s >> m_bScreenSpaceShadows;
  }

  if (uiVersion >= 3)
  {
    s >> m_uiNumCascades;
    s >> m_fMinShadowRange;
    s >> m_fFadeOutStart;
    s >> m_fSplitModeWeight;
    s >> m_fNearPlaneOffset;
  }

  if (uiVersion >= 5)
  {
    s >> m_SourceAngle;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WDirectionalLightComponentPatch_1_2 : public WGraphPatch
{
public:
  WDirectionalLightComponentPatch_1_2()
    : WGraphPatch("WDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("WLightComponent", 2, true);
  }
};

WDirectionalLightComponentPatch_1_2 g_WDirectionalLightComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDirectionalLightVisualizerAttribute, 1, WRTTIDefaultAllocator<WDirectionalLightVisualizerAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDirectionalLightVisualizerAttribute::WDirectionalLightVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WDirectionalLightVisualizerAttribute::WDirectionalLightVisualizerAttribute(const char* szAngleProperty, const char* szColorProperty)
  : WVisualizerAttribute(szAngleProperty, szColorProperty)
{
}



W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);

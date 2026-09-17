#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLensFlareRenderData, 1, WRTTIDefaultAllocator<WLensFlareRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WLensFlareRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const WUInt32 uiTextureIDHash = static_cast<WUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by texture
  m_uiSortingKey = uiTextureIDHash;
}

bool WLensFlareRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WLensFlareRenderData&>(other0);

  return m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WLensFlareElement, WNoBase, 1, WRTTIDefaultAllocator<WLensFlareElement>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("GreyscaleTexture", m_bGreyscaleTexture),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_MEMBER_PROPERTY("ModulateByLightColor", m_bModulateByLightColor)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(10000.0f), new WSuffixAttribute(" m")),
    W_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("ShiftToCenter", m_fShiftToCenter),
    W_MEMBER_PROPERTY("InverseTonemap", m_bInverseTonemap),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WLensFlareElement::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_hTexture;
  inout_stream << m_Color;
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  inout_stream << m_fAspectRatio;
  inout_stream << m_fShiftToCenter;
  inout_stream << m_bInverseTonemap;
  inout_stream << m_bModulateByLightColor;
  inout_stream << m_bGreyscaleTexture;

  return W_SUCCESS;
}

WResult WLensFlareElement::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_hTexture;
  inout_stream >> m_Color;
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  inout_stream >> m_fAspectRatio;
  inout_stream >> m_fShiftToCenter;
  inout_stream >> m_bInverseTonemap;
  inout_stream >> m_bModulateByLightColor;
  inout_stream >> m_bGreyscaleTexture;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WLensFlareComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("LinkToLightShape", GetLinkToLightShape, SetLinkToLightShape)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("LightColor", m_LightColor),
    W_ACCESSOR_PROPERTY("OcclusionSampleRadius", GetOcclusionSampleRadius, SetOcclusionSampleRadius)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(0.1f), new WSuffixAttribute(" m")),
    W_MEMBER_PROPERTY("OcclusionSampleSpread", m_fOcclusionSampleSpread)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f), new WDefaultValueAttribute(0.5f)),
    W_MEMBER_PROPERTY("OcclusionDepthOffset", m_fOcclusionDepthOffset)->AddAttributes(new WSuffixAttribute(" m")),
    W_MEMBER_PROPERTY("ApplyFog", m_bApplyFog)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ARRAY_MEMBER_PROPERTY("Elements", m_Elements)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
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
    new WCategoryAttribute("Rendering"),
    new WSphereManipulatorAttribute("OcclusionSampleRadius"),
    new WSphereVisualizerAttribute("OcclusionSampleRadius", WColor::White)
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WLensFlareComponent::WLensFlareComponent() = default;
WLensFlareComponent::~WLensFlareComponent() = default;

void WLensFlareComponent::OnActivated()
{
  SUPER::OnActivated();

  FindLightComponent();
}

void WLensFlareComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_bDirectionalLight = false;
  m_hLightComponent.Invalidate();
}

void WLensFlareComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_Elements).IgnoreResult();
  s << m_fIntensity;
  s << m_LightColor;
  s << m_fOcclusionSampleRadius;
  s << m_fOcclusionSampleSpread;
  s << m_fOcclusionDepthOffset;
  s << m_bLinkToLightShape;
  s << m_bApplyFog;
}

void WLensFlareComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_Elements).IgnoreResult();
  s >> m_fIntensity;
  if (uiVersion >= 2)
  {
    s >> m_LightColor;
  }
  s >> m_fOcclusionSampleRadius;
  s >> m_fOcclusionSampleSpread;
  s >> m_fOcclusionDepthOffset;
  s >> m_bLinkToLightShape;
  s >> m_bApplyFog;
}

WResult WLensFlareComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_bDirectionalLight)
  {
    ref_bAlwaysVisible = true;
  }
  else
  {
    ref_bounds = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fOcclusionSampleRadius);
  }
  return W_SUCCESS;
}

void WLensFlareComponent::SetLinkToLightShape(bool bLink)
{
  if (m_bLinkToLightShape == bLink)
    return;

  m_bLinkToLightShape = bLink;
  if (IsActiveAndInitialized())
  {
    FindLightComponent();
  }

  TriggerLocalBoundsUpdate();
}

void WLensFlareComponent::SetOcclusionSampleRadius(float fRadius)
{
  m_fOcclusionSampleRadius = fRadius;

  TriggerLocalBoundsUpdate();
}

void WLensFlareComponent::FindLightComponent()
{
  WLightComponent* pLightComponent = nullptr;

  if (m_bLinkToLightShape)
  {
    WGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pLightComponent))
        break;

      pObject = pObject->GetParent();
    }
  }

  if (pLightComponent != nullptr)
  {
    m_bDirectionalLight = pLightComponent->IsInstanceOf<WDirectionalLightComponent>();
    m_hLightComponent = pLightComponent->GetHandle();
  }
  else
  {
    m_bDirectionalLight = false;
    m_hLightComponent.Invalidate();
  }
}

void WLensFlareComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);
}

void WLensFlareComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't render in shadow and reflection views
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Reflection)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  const WCamera* pCamera = msg.m_pView->GetCamera();
  WTransform globalTransform = GetOwner()->GetGlobalTransform();
  WBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  float fScale = globalTransform.GetMaxScale();
  WColor lightColor = WColor::White;

  const WLightComponent* pLightComponent = nullptr;
  if (GetWorld()->TryGetComponent(m_hLightComponent, pLightComponent))
  {
    lightColor = pLightComponent->GetLightColor();
    lightColor *= pLightComponent->GetIntensity() * 0.1f;
  }
  else
  {
    lightColor = m_LightColor;
  }

  float fFade = 1.0f;
  if (auto pDirectionalLight = WDynamicCast<const WDirectionalLightComponent*>(pLightComponent))
  {
    WTransform localOffset = WTransform::MakeIdentity();
    localOffset.m_vPosition = WVec3(pCamera->GetFarPlane() * -0.999f, 0, 0);

    globalTransform = WTransform::MakeGlobalTransform(globalTransform, localOffset);
    globalTransform.m_vPosition += pCamera->GetCenterPosition();

    if (pCamera->IsPerspective())
    {
      float fHalfHeight = WMath::Tan(pCamera->GetFovY(1.0f) * 0.5f) * pCamera->GetFarPlane();
      fScale *= fHalfHeight;
    }

    lightColor *= 10.0f;
  }
  else if (auto pSpotLight = WDynamicCast<const WSpotLightComponent*>(pLightComponent))
  {
    const WVec3 lightDir = globalTransform.TransformDirection(WVec3::MakeAxisX());
    const WVec3 cameraDir = (pCamera->GetCenterPosition() - globalTransform.m_vPosition).GetNormalized();

    const float cosAngle = lightDir.Dot(cameraDir);
    const float fCosInner = WMath::Cos(pSpotLight->GetInnerSpotAngle() * 0.5f);
    const float fCosOuter = WMath::Cos(pSpotLight->GetOuterSpotAngle() * 0.5f);
    fFade = WMath::Saturate((cosAngle - fCosOuter) / WMath::Max(0.001f, (fCosInner - fCosOuter)));
    fFade *= fFade;
  }

  for (auto& element : m_Elements)
  {
    if (element.m_hTexture.IsValid() == false)
      continue;

    WColor color = element.m_Color * m_fIntensity;
    if (element.m_bModulateByLightColor)
    {
      color *= lightColor;
    }
    color.a = element.m_Color.a * fFade;

    if (color.GetLuminance() <= 0.0f || color.a <= 0.0f)
      continue;

    WLensFlareRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WLensFlareRenderData>(GetOwner());
    {
      pRenderData->m_vGlobalPosition = globalTransform.m_vPosition;

      pRenderData->m_hTexture = element.m_hTexture;
      pRenderData->m_Color = color.GetAsVec4();
      pRenderData->m_fSize = element.m_fSize * fScale;
      pRenderData->m_fMaxScreenSize = element.m_fMaxScreenSize * 2.0f;
      pRenderData->m_fAspectRatio = 1.0f / element.m_fAspectRatio;
      pRenderData->m_fShiftToCenter = element.m_fShiftToCenter;
      pRenderData->m_fOcclusionSampleRadius = m_fOcclusionSampleRadius * fScale;
      pRenderData->m_fOcclusionSampleSpread = m_fOcclusionSampleSpread;
      pRenderData->m_fOcclusionDepthOffset = m_fOcclusionDepthOffset * fScale;
      pRenderData->m_bInverseTonemap = element.m_bInverseTonemap;
      pRenderData->m_bGreyscaleTexture = element.m_bGreyscaleTexture;
      pRenderData->m_bApplyFog = m_bApplyFog;

      pRenderData->FillSortingKey();
    }

    const bool bIsSecondaryFlare = element.m_fShiftToCenter != 0.0f;
    const auto category = bIsSecondaryFlare ? WDefaultRenderDataCategories::LensEffects : WDefaultRenderDataCategories::LitTransparent;

    msg.AddRenderData(pRenderData, category, pLightComponent != nullptr ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareComponent);

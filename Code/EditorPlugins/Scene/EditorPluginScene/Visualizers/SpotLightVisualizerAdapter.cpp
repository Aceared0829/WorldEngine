#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WSpotLightVisualizerAdapter::WSpotLightVisualizerAdapter() = default;

WSpotLightVisualizerAdapter::~WSpotLightVisualizerAdapter() = default;

void WSpotLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Cone, WColor::White, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);
  m_hRadiusGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Sphere, WColor::White, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  pAssetDocument->AddSyncObject(&m_hRadiusGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hRadiusGizmo.SetVisible(false);
}

void WSpotLightVisualizerAdapter::Update()
{
  const WSpotLightVisualizerAttribute* pAttr = static_cast<const WSpotLightVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WAngle>(), "Invalid property bound to WSpotLightVisualizerAttribute 'angle'");
    m_fAngleScale = WMath::Tan(value.ConvertTo<WAngle>() * 0.5f);
  }

  WColor color = WColor::White;
  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WSpotLightVisualizerAttribute 'color'");
    color = value.ConvertTo<WColor>();
  }
  m_hGizmo.SetColor(color);
  m_hRadiusGizmo.SetColor(color);

  m_fScale = 1.0f;
  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    WVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    W_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to WSpotLightVisualizerAttribute 'range'");

    WVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    W_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to WSpotLightVisualizerAttribute 'intensity'");

    m_fScale = WLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  m_fRadius = 0.0f;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to WSpotLightVisualizerAttribute 'radius'");
    m_fRadius = value.ConvertTo<float>();
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale != 0.0f && m_fScale != 0.0f);
  m_hRadiusGizmo.SetVisible(m_bVisualizerIsVisible && m_fRadius > 0.0f);
}

void WSpotLightVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(WVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fScale);
  m_hGizmo.SetTransformation(t);

  WTransform tRadius = GetObjectTransform();
  tRadius.m_vScale *= m_fRadius;
  m_hRadiusGizmo.SetTransformation(tRadius);
}

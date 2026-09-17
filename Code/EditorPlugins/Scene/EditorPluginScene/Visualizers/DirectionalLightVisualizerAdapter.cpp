#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/DirectionalLightVisualizerAdapter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WDirectionalLightVisualizerAdapter::WDirectionalLightVisualizerAdapter() = default;

WDirectionalLightVisualizerAdapter::~WDirectionalLightVisualizerAdapter() = default;

void WDirectionalLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Cone, WColor::White, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WDirectionalLightVisualizerAdapter::Update()
{
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WDirectionalLightVisualizerAttribute* pAttr = static_cast<const WDirectionalLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fAngleScale = 0.0f;

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.CanConvertTo<WAngle>(), "Invalid property bound to WDirectionalLightVisualizerAttribute 'angle'");
    m_fAngleScale = WMath::Tan(value.ConvertTo<WAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WDirectionalLightVisualizerAdapter 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>());
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale > 0.0f);
}

void WDirectionalLightVisualizerAdapter::UpdateGizmoTransform()
{
  // The cone's length is a purely symbolic value since directional lights have no position.
  // The opening angle is what matters: tan(halfAngle) in the lateral axes scales the cone's
  // base radius relative to its length, which matches the way the spot light visualizer works.
  constexpr float fSymbolicLength = -1.0f;

  WTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(WVec3(1.0f, m_fAngleScale, m_fAngleScale) * fSymbolicLength);
  m_hGizmo.SetTransformation(t);
}

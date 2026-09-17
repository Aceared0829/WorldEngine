#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WConeVisualizerAdapter::WConeVisualizerAdapter() = default;

WConeVisualizerAdapter::~WConeVisualizerAdapter() = default;

void WConeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WConeVisualizerAttribute* pAttr = static_cast<const WConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Cone, pAttr->m_Color, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WConeVisualizerAdapter::Update()
{
  const WConeVisualizerAttribute* pAttr = static_cast<const WConeVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WAngle>(), "Invalid property bound to WConeVisualizerAttribute 'angle'");
    m_fAngleScale = WMath::Tan(value.ConvertTo<WAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WConeVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>());
  }

  m_fFinalScale = pAttr->m_fScale;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WConeVisualizerAttribute 'radius'");
    m_fFinalScale *= value.ConvertTo<float>();
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale != 0.0f && m_fFinalScale != 0.0f);
}

void WConeVisualizerAdapter::UpdateGizmoTransform()
{
  const WConeVisualizerAttribute* pAttr = static_cast<const WConeVisualizerAttribute*>(m_pVisualizerAttr);

  const WQuat axisRotation = WBasisAxis::GetBasisRotation_PosX(pAttr->m_Axis);

  WTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(WVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fFinalScale);
  t.m_qRotation = axisRotation * t.m_qRotation;
  m_hGizmo.SetTransformation(t);
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WCylinderVisualizerAdapter::WCylinderVisualizerAdapter() = default;

WCylinderVisualizerAdapter::~WCylinderVisualizerAdapter() = default;

void WCylinderVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WCylinderVisualizerAttribute* pAttr = static_cast<const WCylinderVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, WEngineGizmoHandleType::CylinderZ, pAttr->m_Color, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hCylinder);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
}

void WCylinderVisualizerAdapter::Update()
{
  const WCylinderVisualizerAttribute* pAttr = static_cast<const WCylinderVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    W_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to WCylinderVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    WVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to WCylinderVisualizerAttribute 'radius'",
      pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCylinderVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WCylinderVisualizerAttribute 'color'");
    m_hCylinder.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WVec3>(), "Invalid property bound to WCylinderVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<WVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<WVec3>());
  }

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WInt32>(), "Invalid property bound to WCylinderVisualizerAttribute 'axis'");

    m_Axis = static_cast<WBasisAxis::Enum>(value.ConvertTo<WInt32>());
  }
  else
  {
    m_Axis = pAttr->m_Axis;
  }

  m_Anchor = pAttr->m_Anchor;
}

void WCylinderVisualizerAdapter::UpdateGizmoTransform()
{
  const WQuat axisRotation = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, m_Axis);

  WTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = WVec3(m_fRadius, m_fRadius, m_fHeight);
  t.m_vPosition = m_vPositionOffset;

  WVec3 vOffset = WVec3::MakeZero();

  if (m_Anchor.IsSet(WVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  // WTransform doesn't (can't) combine rotations with scales
  // however, here we know that the axisRotation is just an axis remapping, so we can combine them
  WTransform parentTransform = GetObjectTransform();
  WTransform newTrans = parentTransform * t;
  newTrans.m_vScale = (axisRotation * parentTransform.m_vScale).CompMul(t.m_vScale);

  m_hCylinder.SetTransformation(newTrans);
}

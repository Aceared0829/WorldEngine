#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WCapsuleVisualizerAdapter::WCapsuleVisualizerAdapter() = default;
WCapsuleVisualizerAdapter::~WCapsuleVisualizerAdapter() = default;

void WCapsuleVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");
  W_MSVC_ANALYSIS_ASSUME(pAssetDocument != nullptr);

  const WCapsuleVisualizerAttribute* pAttr = static_cast<const WCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, WEngineGizmoHandleType::CylinderZ, pAttr->m_Color, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);
  m_hSphereTop.ConfigureHandle(nullptr, WEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);
  m_hSphereBottom.ConfigureHandle(nullptr, WEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hCylinder);
  pAssetDocument->AddSyncObject(&m_hSphereTop);
  pAssetDocument->AddSyncObject(&m_hSphereBottom);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);
}

void WCapsuleVisualizerAdapter::Update()
{
  const WCapsuleVisualizerAttribute* pAttr = static_cast<const WCapsuleVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;
  m_Anchor = pAttr->m_Anchor;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    W_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to WCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    WVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to WCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCapsuleVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WCapsuleVisualizerAttribute 'color'");
    m_hSphereTop.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
    m_hSphereBottom.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
    m_hCylinder.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }
}

void WCapsuleVisualizerAdapter::UpdateGizmoTransform()
{
  WVec3 vOffset = WVec3::MakeZero();

  if (m_Anchor.IsSet(WVisualizerAnchor::PosX))
    vOffset.x -= m_fRadius;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegX))
    vOffset.x += m_fRadius;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosY))
    vOffset.y -= m_fRadius;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegY))
    vOffset.y += m_fRadius;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosZ))
    vOffset.z -= m_fRadius + 0.5f * m_fHeight;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegZ))
    vOffset.z += m_fRadius + 0.5f * m_fHeight;

  WTransform tSphereTop;
  tSphereTop.SetIdentity();
  tSphereTop.m_vScale = WVec3(m_fRadius);
  tSphereTop.m_vPosition.z = m_fHeight * 0.5f;
  tSphereTop.m_vPosition += vOffset;

  WTransform tSphereBottom;
  tSphereBottom.SetIdentity();
  tSphereBottom.m_vScale = WVec3(m_fRadius, -m_fRadius, -m_fRadius);
  tSphereBottom.m_vPosition.z = -m_fHeight * 0.5f;
  tSphereBottom.m_vPosition += vOffset;

  WTransform tCylinder;
  tCylinder.SetIdentity();
  tCylinder.m_vScale = WVec3(m_fRadius, m_fRadius, m_fHeight);
  tCylinder.m_vPosition += vOffset;

  m_hSphereTop.SetTransformation(GetObjectTransform() * tSphereTop);
  m_hSphereBottom.SetTransformation(GetObjectTransform() * tSphereBottom);
  m_hCylinder.SetTransformation(GetObjectTransform() * tCylinder);
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WSphereVisualizerAdapter::WSphereVisualizerAdapter() = default;
WSphereVisualizerAdapter::~WSphereVisualizerAdapter() = default;

void WSphereVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WSphereVisualizerAttribute* pAttr = static_cast<const WSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Sphere, pAttr->m_Color, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WSphereVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WSphereVisualizerAttribute* pAttr = static_cast<const WSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WSphereVisualizerAttribute 'radius'");
    m_fScale = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WSphereVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WVec3>(), "Invalid property bound to WSphereVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<WVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<WVec3>());
  }

  m_Anchor = pAttr->m_Anchor;
}

void WSphereVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t;
  t.m_qRotation.SetIdentity();
  t.m_vScale.Set(m_fScale);
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
    vOffset.z -= t.m_vScale.z;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}

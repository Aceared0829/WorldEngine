#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WBoxVisualizerAdapter::WBoxVisualizerAdapter() = default;
WBoxVisualizerAdapter::~WBoxVisualizerAdapter() = default;

void WBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WBoxVisualizerAttribute* pAttr = static_cast<const WBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineBox, pAttr->m_Color, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WBoxVisualizerAdapter::Update()
{
  const WBoxVisualizerAttribute* pAttr = static_cast<const WBoxVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(pAttr->m_fSizeScale);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    WVariant val;
    if (pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetSizeProperty()), val).Succeeded())
    {
      if (val.IsNumber())
      {
        m_vScale *= val.ConvertTo<float>();
      }
      else if (val.CanConvertTo<WVec3>())
      {
        m_vScale *= val.ConvertTo<WVec3>();
      }
      else if (val.CanConvertTo<WVec2>())
      {
        m_vScale *= val.ConvertTo<WVec2>().GetAsVec3(1);
      }
    }
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WBoxVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WVec3>(), "Invalid property bound to WBoxVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<WVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<WVec3>());
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<WQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  m_Anchor = pAttr->m_Anchor;
}

void WBoxVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  WVec3 vOffset = WVec3::MakeZero();

  if (m_Anchor.IsSet(WVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(WVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}

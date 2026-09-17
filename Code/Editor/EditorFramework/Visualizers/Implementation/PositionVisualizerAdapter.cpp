#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/PositionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WPositionVisualizerAdapter::WPositionVisualizerAdapter() = default;
WPositionVisualizerAdapter::~WPositionVisualizerAdapter() = default;

void WPositionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WPositionVisualizerAttribute* pAttr = static_cast<const WPositionVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Cross, pAttr->m_Color, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WPositionVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WPositionVisualizerAttribute* pAttr = static_cast<const WPositionVisualizerAttribute*>(m_pVisualizerAttr);

  m_vPosition = WVec3::MakeZero();
  m_fScale = pAttr->m_fSizeScale;

  if (!pAttr->GetPositionProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPositionProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WVec3>(), "Invalid property bound to WPositionVisualizerAttribute 'position'");
    m_vPosition = value.ConvertTo<WVec3>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WPositionVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }
}

void WPositionVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t;
  t.SetIdentity();
  t.m_vScale.Set(m_fScale);
  t.m_vPosition = m_vPosition;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}

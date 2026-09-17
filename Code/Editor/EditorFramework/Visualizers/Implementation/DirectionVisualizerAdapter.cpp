#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WDirectionVisualizerAdapter::WDirectionVisualizerAdapter() = default;

WDirectionVisualizerAdapter::~WDirectionVisualizerAdapter() = default;

void WDirectionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WDirectionVisualizerAttribute* pAttr = static_cast<const WDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Arrow, pAttr->m_Color, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WDirectionVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  const WDirectionVisualizerAttribute* pAttr = static_cast<const WDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WDirectionVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<WColor>() * pAttr->m_Color);
  }
}

void WDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  const WDirectionVisualizerAttribute* pAttr = static_cast<const WDirectionVisualizerAttribute*>(m_pVisualizerAttr);
  float fScale = pAttr->m_fScale;

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WDirectionVisualizerAttribute 'length'");
    fScale *= value.ConvertTo<float>();
  }

  WVec3 axis = WBasisAxis::GetBasisVector(pAttr->m_Axis);

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).AssertSuccess();

    if (value.IsA<WVec3>())
    {
      axis = value.ConvertTo<WVec3>();
      fScale *= axis.GetLength();
      axis.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();
    }
    else
    {
      W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WInt32>(), "Invalid property bound to WDirectionVisualizerAttribute 'length'");

      axis = WBasisAxis::GetBasisVector(static_cast<WBasisAxis::Enum>(value.ConvertTo<WInt32>()));
    }
  }

  const WQuat axisRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), axis);

  WTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = WVec3(fScale);
  t.m_vPosition = axisRotation * WVec3(fScale * 0.5f, 0, 0);

  WTransform tObject = GetObjectTransform();
  tObject.m_vScale.Set(1.0f);

  m_hGizmo.SetTransformation(tObject * t);
}

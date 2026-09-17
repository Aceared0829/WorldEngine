#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WBoxReflectionProbeVisualizerAdapter::WBoxReflectionProbeVisualizerAdapter() = default;
WBoxReflectionProbeVisualizerAdapter::~WBoxReflectionProbeVisualizerAdapter() = default;

void WBoxReflectionProbeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineBox, WColorScheme::LightUI(WColorScheme::Yellow), WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WBoxReflectionProbeVisualizerAdapter::Update()
{
  const WBoxReflectionProbeVisualizerAttribute* pAttr = static_cast<const WBoxReflectionProbeVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(1.0f);
  WVec3 influenceScale;
  WVec3 influenceShift;

  if (!pAttr->GetExtentsProperty().IsEmpty())
  {
    m_vScale = pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetExtentsProperty()));
  }

  if (!pAttr->GetInfluenceScaleProperty().IsEmpty())
  {
    influenceScale = pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetInfluenceScaleProperty()));
  }

  if (!pAttr->GetInfluenceShiftProperty().IsEmpty())
  {
    influenceShift = pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetInfluenceShiftProperty()));
  }

  m_vPositionOffset = m_vScale.CompMul(influenceShift.CompMul(WVec3(1.0f) - influenceScale)) * 0.5f;
  m_vScale *= influenceScale;

  m_qRotation.SetIdentity();
}

void WBoxReflectionProbeVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}

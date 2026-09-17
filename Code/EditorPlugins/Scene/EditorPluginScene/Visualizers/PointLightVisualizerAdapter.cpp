#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WPointLightVisualizerAdapter::WPointLightVisualizerAdapter() = default;

WPointLightVisualizerAdapter::~WPointLightVisualizerAdapter() = default;

void WPointLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  // Sphere gizmo shows the effective attenuation range. For tube lights it encompasses the full capsule.
  m_hRangeGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Sphere, WColor::White, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);
  m_hCapsuleL.ConfigureHandle(nullptr, WEngineGizmoHandleType::HalfSphereZ, WColor::White, WGizmoFlags::Visualizer);
  m_hCapsuleR.ConfigureHandle(nullptr, WEngineGizmoHandleType::HalfSphereZ, WColor::White, WGizmoFlags::Visualizer);
  m_hCapsuleM.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineCylinderZ, WColor::White, WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hRangeGizmo);
  pAssetDocument->AddSyncObject(&m_hCapsuleL);
  pAssetDocument->AddSyncObject(&m_hCapsuleR);
  pAssetDocument->AddSyncObject(&m_hCapsuleM);

  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleL.SetVisible(false);
  m_hCapsuleR.SetVisible(false);
  m_hCapsuleM.SetVisible(false);
}

void WPointLightVisualizerAdapter::Update()
{
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WPointLightVisualizerAttribute* pAttr = static_cast<const WPointLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fDisplayRange = 1.0f;
  m_fLength = 0.0f;
  m_fRadius = 0.0f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    WVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    W_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to WPointLightVisualizerAttribute 'range'");

    WVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    W_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to WPointLightVisualizerAttribute 'intensity'");

    m_fDisplayRange = WMath::Max(range.ConvertTo<float>(), WLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>()));
  }

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to WPointLightVisualizerAttribute 'length'");
    m_fLength = value.ConvertTo<float>();
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to WPointLightVisualizerAttribute 'radius'");
    m_fRadius = value.ConvertTo<float>();
  }

  WColor color = WColor::White;
  if (!pAttr->GetColorProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WColor>(), "Invalid property bound to WPointLightVisualizerAdapter 'color'");
    color = value.ConvertTo<WColor>();
  }

  m_bIsTube = (m_fLength > 0.0f || m_fRadius > 0.0f);

  m_hRangeGizmo.SetColor(color);
  m_hCapsuleL.SetColor(color);
  m_hCapsuleR.SetColor(color);
  m_hCapsuleM.SetColor(color);

  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleL.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
  m_hCapsuleR.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
  m_hCapsuleM.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
}

void WPointLightVisualizerAdapter::UpdateGizmoTransform()
{
  // Range sphere encompasses the full attenuation volume (includes half the tube length when tubed).
  WTransform t = GetObjectTransform();
  const float fBoundingRadius = m_fDisplayRange + m_fLength * 0.5f;
  t.m_vScale *= fBoundingRadius;
  m_hRangeGizmo.SetTransformation(t);

  if (!m_bIsTube)
    return;

  const WQuat rotToX = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, WBasisAxis::PositiveX);
  const WQuat rot180 = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, WBasisAxis::NegativeZ);

  WTransform baseTransform = GetObjectTransform();
  baseTransform.m_qRotation = baseTransform.m_qRotation * rotToX;

  // Clamp to tiny non-zero scales so the gizmo primitives remain renderable when only one of length/radius is set.
  const float fLength = WMath::Max(m_fLength, 0.001f);
  const float fRadius = WMath::Max(m_fRadius, 0.001f);

  WTransform tMid = baseTransform;
  tMid.m_vScale.x = fRadius;
  tMid.m_vScale.y = fRadius;
  tMid.m_vScale.z = fLength;
  m_hCapsuleM.SetTransformation(tMid);

  WTransform tCap = baseTransform;
  tCap.m_vScale.Set(fRadius);

  tCap.m_vPosition = baseTransform.m_vPosition + baseTransform.m_qRotation * WVec3(0, 0, fLength * 0.5f);
  m_hCapsuleL.SetTransformation(tCap);

  tCap.m_vPosition = baseTransform.m_vPosition - baseTransform.m_qRotation * WVec3(0, 0, fLength * 0.5f);
  tCap.m_qRotation = tCap.m_qRotation * rot180;
  m_hCapsuleR.SetTransformation(tCap);
}

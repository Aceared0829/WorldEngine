#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WCameraVisualizerAdapter::WCameraVisualizerAdapter() = default;

WCameraVisualizerAdapter::~WCameraVisualizerAdapter() = default;

void WCameraVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  m_hBoxGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineBox, WColor::DodgerBlue, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);
  m_hFrustumGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::Frustum, WColor::DodgerBlue, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);
  m_hNearPlaneGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineRect, WColor::LightBlue, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);
  m_hFarPlaneGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineRect, WColor::PaleVioletRed, WGizmoFlags::Visualizer | WGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hBoxGizmo);
  pAssetDocument->AddSyncObject(&m_hFrustumGizmo);
  pAssetDocument->AddSyncObject(&m_hNearPlaneGizmo);
  pAssetDocument->AddSyncObject(&m_hFarPlaneGizmo);

  m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
}

void WCameraVisualizerAdapter::Update()
{

  const WCameraVisualizerAttribute* pAttr = static_cast<const WCameraVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fNearPlane = 1.0f;
  float fFarPlane = 10.0f;
  WInt32 iMode = 0;

  if (!pAttr->GetModeProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetModeProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<WInt32>(), "Invalid property bound to WCameraVisualizerAttribute 'mode'");
    iMode = value.ConvertTo<WInt32>();
  }

  if (!pAttr->GetNearPlaneProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetNearPlaneProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCameraVisualizerAttribute 'near plane'");
    fNearPlane = value.ConvertTo<float>();
  }

  if (!pAttr->GetFarPlaneProperty().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFarPlaneProperty()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCameraVisualizerAttribute 'far plane'");
    fFarPlane = value.ConvertTo<float>();
  }

  if (iMode == WCameraMode::OrthoFixedHeight || iMode == WCameraMode::OrthoFixedWidth)
  {
    float fDimensions = 1.0f;

    if (!pAttr->GetOrthoDimProperty().IsEmpty())
    {
      WVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOrthoDimProperty()), value).AssertSuccess();

      W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCameraVisualizerAttribute 'ortho dim'");
      fDimensions = value.ConvertTo<float>();
    }

    {
      const float fRange = fFarPlane - fNearPlane;

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fRange, fDimensions, fDimensions);
      m_LocalTransformFrustum.m_vPosition.Set(fNearPlane + fRange * 0.5f, 0, 0);
    }

    m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFrustumGizmo.SetVisible(false);
    m_hNearPlaneGizmo.SetVisible(false);
    m_hFarPlaneGizmo.SetVisible(false);

    m_LocalTransformNearPlane.SetIdentity();
    m_LocalTransformFarPlane.SetIdentity();
  }
  else
  {
    float fFOV = 45.0f;

    if (!pAttr->GetFovProperty().IsEmpty())
    {
      WVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFovProperty()), value).AssertSuccess();

      W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WCameraVisualizerAttribute 'fov'");
      fFOV = value.ConvertTo<float>();
    }

    {
      const float fAngleScale = WMath::Tan(WAngle::MakeFromDegree(fFOV) * 0.5f);
      const float fFrustumScale = WMath::Min(fFarPlane, 10.0f);
      const float fFarPlaneScale = WMath::Min(fFarPlane, 9.0f);
      ;

      // indicate whether the shown far plane is the actual distance, or just the maximum visualization distance
      m_hFarPlaneGizmo.SetColor(fFarPlane > 9.0f ? WColor::DodgerBlue : WColor::PaleVioletRed);

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fFrustumScale, fAngleScale * fFrustumScale, fAngleScale * fFrustumScale);
      m_LocalTransformFrustum.m_vPosition.Set(0, 0, 0);

      m_LocalTransformNearPlane.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(90));
      m_LocalTransformNearPlane.m_vScale.Set(fAngleScale * fNearPlane, fAngleScale * fNearPlane, 1);
      m_LocalTransformNearPlane.m_vPosition.Set(fNearPlane, 0, 0);

      m_LocalTransformFarPlane.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(90));
      m_LocalTransformFarPlane.m_vScale.Set(fAngleScale * fFarPlaneScale, fAngleScale * fFarPlaneScale, 1);
      m_LocalTransformFarPlane.m_vPosition.Set(fFarPlaneScale, 0, 0);
    }

    m_hBoxGizmo.SetVisible(false);
    m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  }
}

void WCameraVisualizerAdapter::UpdateGizmoTransform()
{
  WTransform t = GetObjectTransform();
  m_hBoxGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hFrustumGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hNearPlaneGizmo.SetTransformation(t * m_LocalTransformNearPlane);
  m_hFarPlaneGizmo.SetTransformation(t * m_LocalTransformFarPlane);
}

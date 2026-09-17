#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WNonUniformBoxGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WNonUniformBoxGizmo::WNonUniformBoxGizmo()
{
  m_vNegSize.Set(1.0f);
  m_vPosSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hOutline.ConfigureHandle(this, WEngineGizmoHandleType::LineBox, WColorScheme::LightUI(WColorScheme::Gray), WGizmoFlags::ShowInOrtho);

  WColor cols[6] = {
    WColorScheme::LightUI(WColorScheme::Red),
    WColorScheme::LightUI(WColorScheme::Red),
    WColorScheme::LightUI(WColorScheme::Green),
    WColorScheme::LightUI(WColorScheme::Green),
    WColorScheme::LightUI(WColorScheme::Blue),
    WColorScheme::LightUI(WColorScheme::Blue),
  };

  for (WUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].ConfigureHandle(this, WEngineGizmoHandleType::Box, cols[i], WGizmoFlags::ConstantSize | WGizmoFlags::OnTop | WGizmoFlags::ShowInOrtho | WGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WNonUniformBoxGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOutline);

  for (WUInt32 i = 0; i < 6; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Nobs[i]);
  }
}

void WNonUniformBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hOutline.SetVisible(bVisible);

  for (WUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].SetVisible(bVisible);
  }
}

void WNonUniformBoxGizmo::OnTransformationChanged(const WTransform& transform)
{
  WMat4 scale, rot;
  scale = WMat4::MakeScaling(m_vNegSize + m_vPosSize);

  const WVec3 center = WMath::Lerp(-m_vNegSize, m_vPosSize, 0.5f);

  scale.SetTranslationVector(center);
  scale = transform.GetAsMat4() * scale;

  m_hOutline.SetTransformation(scale);

  for (WUInt32 i = 0; i < 6; ++i)
  {
    WVec3 pos = center;
    WVec3 dir;

    switch (i)
    {
      case ManipulateMode::DragNegX:
        pos.x = -m_vNegSize.x;
        dir.Set(-1, 0, 0);
        break;
      case ManipulateMode::DragPosX:
        pos.x = m_vPosSize.x;
        dir.Set(+1, 0, 0);
        break;
      case ManipulateMode::DragNegY:
        pos.y = -m_vNegSize.y;
        dir.Set(0, -1, 0);
        break;
      case ManipulateMode::DragPosY:
        pos.y = m_vPosSize.y;
        dir.Set(0, +1, 0);
        break;
      case ManipulateMode::DragNegZ:
        pos.z = -m_vNegSize.z;
        dir.Set(0, 0, -1);
        break;
      case ManipulateMode::DragPosZ:
        pos.z = m_vPosSize.z;
        dir.Set(0, 0, +1);
        break;
    }
    pos = pos.CompMul(transform.m_vScale);

    WTransform t;
    t.m_qRotation = transform.m_qRotation;
    t.m_vPosition = transform.m_vPosition + t.m_qRotation * pos;
    t.m_vScale.Set(0.15f);

    m_vMainAxis[i] = t.m_qRotation * dir;

    m_Nobs[i].SetTransformation(t);
  }
}

void WNonUniformBoxGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WNonUniformBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  for (WUInt32 i = 0; i < 6; ++i)
  {
    if (m_pInteractionGizmoHandle == &m_Nobs[i])
    {
      m_ManipulateMode = (ManipulateMode)i;
      m_vMoveAxis = m_vMainAxis[i];
      m_vStartPosition = m_pInteractionGizmoHandle->GetTransformation().m_vPosition;
      goto modify;
    }
  }

  return WEditorInput::MayBeHandledByOthers;

modify:

{
  WMat4 mView = m_pCamera->GetViewMatrix();
  WMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  WMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();
}

  m_vStartNegSize = m_vNegSize;
  m_vStartPosSize = m_vPosSize;

  GetPointOnAxis(e->pos().x(), e->pos().y(), m_vInteractionPivot).IgnoreResult();

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = WTime::Now();

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::Normal);

  SetActiveInputContext(this);

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WNonUniformBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WNonUniformBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  m_vNegSize = m_vStartNegSize;
  m_vPosSize = m_vStartPosSize;

  {
    WVec3 vCurrentInteractionPoint;

    if (GetPointOnAxis(e->pos().x(), e->pos().y(), vCurrentInteractionPoint).Failed())
    {
      m_vLastMousePos = UpdateMouseMode(e);
      return WEditorInput::WasExclusivelyHandled;
    }

    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const WVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const WVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    WVec3 vTranslate = GetTransformation().m_qRotation.GetInverse() * (vNewPos - m_vStartPosition);

    // disable snapping when SHIFT is pressed
    if (!e->modifiers().testFlag(Qt::ShiftModifier))
    {
      WSnapProvider::SnapTranslation(vTranslate);
    }

    switch (m_ManipulateMode)
    {
      case None:
        break;
      case DragNegX:
        m_vNegSize.x -= vTranslate.x;
        if (m_bLinkAxis)
          m_vPosSize.x -= vTranslate.x;
        break;
      case DragPosX:
        m_vPosSize.x += vTranslate.x;
        if (m_bLinkAxis)
          m_vNegSize.x += vTranslate.x;
        break;
      case DragNegY:
        m_vNegSize.y -= vTranslate.y;
        if (m_bLinkAxis)
          m_vPosSize.y -= vTranslate.y;
        break;
      case DragPosY:
        m_vPosSize.y += vTranslate.y;
        if (m_bLinkAxis)
          m_vNegSize.y += vTranslate.y;
        break;
      case DragNegZ:
        m_vNegSize.z -= vTranslate.z;
        if (m_bLinkAxis)
          m_vPosSize.z -= vTranslate.z;
        break;
      case DragPosZ:
        m_vPosSize.z += vTranslate.z;
        if (m_bLinkAxis)
          m_vNegSize.z += vTranslate.z;
        break;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WNonUniformBoxGizmo::SetSize(const WVec3& vNegSize, const WVec3& vPosSize, bool bLinkAxis)
{
  m_vNegSize = vNegSize;
  m_vPosSize = vPosSize;
  m_bLinkAxis = bLinkAxis;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

WResult WNonUniformBoxGizmo::GetPointOnAxis(WInt32 iScreenPosX, WInt32 iScreenPosY, WVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  WVec3 vPos, vRayDir;
  if (WGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, WVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return W_FAILURE;

  const WVec3 vDir = m_pCamera->GetDirForwards();

  if (WMath::Abs(vDir.Dot(m_vMoveAxis)) > 0.999f)
    return W_FAILURE;

  const WVec3 vPlaneTangent = m_vMoveAxis.CrossRH(vDir).GetNormalized();
  const WVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

  WPlane Plane;
  Plane = WPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  WVec3 vIntersection;
  if (m_pCamera->IsPerspective())
  {
    if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
      return W_FAILURE;
  }
  else
  {
    if (!Plane.GetRayIntersectionBiDirectional(vPos - vRayDir, vRayDir, nullptr, &vIntersection))
      return W_FAILURE;
  }

  const WVec3 vDirAlongRay = vIntersection - m_vStartPosition;
  const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return W_SUCCESS;
}

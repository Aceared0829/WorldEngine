#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTranslateGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WTranslateGizmo::WTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  const WColor colr = WColorScheme::LightUI(WColorScheme::Red);
  const WColor colg = WColorScheme::LightUI(WColorScheme::Green);
  const WColor colb = WColorScheme::LightUI(WColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowX.obj");
  m_hAxisY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowZ.obj");

  m_hPlaneYZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable | WGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneX.obj");
  m_hPlaneXZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable | WGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneY.obj");
  m_hPlaneXY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable | WGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_LastHandleInteraction = HandleInteraction::None;
}

void WTranslateGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);

  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneYZ);
}

void WTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);

  m_hPlaneXY.SetVisible(bVisible);
  m_hPlaneXZ.SetVisible(bVisible);
  m_hPlaneYZ.SetVisible(bVisible);
}

void WTranslateGizmo::OnTransformationChanged(const WTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hPlaneXY.SetTransformation(transform);
  m_hPlaneYZ.SetTransformation(transform);
  m_hPlaneXZ.SetTransformation(transform);

  if (!IsActiveInputContext())
  {
    // if the gizmo is currently not being dragged, copy the translation into the start position
    m_vStartPosition = GetTransformation().m_vPosition;
  }
}

void WTranslateGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(true);
  m_hAxisY.SetVisible(true);
  m_hAxisZ.SetVisible(true);

  m_hPlaneXY.SetVisible(true);
  m_hPlaneXZ.SetVisible(true);
  m_hPlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
  m_LastHandleInteraction = HandleInteraction::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetOwnerWindow()->SetPermanentStatusBarMsg("");
}

WEditorInput WTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  const WQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis = gizmoRot * WVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis = gizmoRot * WVec3(0, 1, 0);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis = gizmoRot * WVec3(0, 0, 1);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXY)
  {
    m_vMoveAxis = gizmoRot * WVec3(0, 0, 1);
    m_vPlaneAxis[0] = gizmoRot * WVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * WVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXZ)
  {
    m_vMoveAxis = gizmoRot * WVec3(0, 1, 0);
    m_vPlaneAxis[0] = gizmoRot * WVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * WVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneYZ)
  {
    m_vMoveAxis = gizmoRot * WVec3(1, 0, 0);
    m_vPlaneAxis[0] = gizmoRot * WVec3(0, 1, 0);
    m_vPlaneAxis[1] = gizmoRot * WVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneX;
  }
  else
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetInverseViewProjectionMatrix(m_mInvViewProj);

  m_LastInteraction = WTime::Now();

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::WrapAtScreenBorders);
  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(m_vStartPosition, m_vMoveAxis, WVec2I32(e->pos().x(), e->pos().y()), m_mInvViewProj, m_vInteractionPivot).IgnoreResult();
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(WVec2I32(e->pos().x(), e->pos().y()), m_vInteractionPivot).IgnoreResult();
  }

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WTranslateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WResult WTranslateGizmo::GetPointOnPlane(const WVec2I32& vScreenPos, WVec3& out_Result) const
{
  WPlane Plane;
  Plane = WPlane::MakeFromNormalAndPoint(m_vMoveAxis, m_vStartPosition);

  return SUPER::GetPointOnPlane(Plane, vScreenPos, m_mInvViewProj, out_Result);
}

WEditorInput WTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const WVec2I32 CurMousePos(mousePosition.x(), mousePosition.y());

  m_LastInteraction = tNow;

  WTransform mTrans = GetTransformation();
  WVec3 vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    WVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(m_vStartPosition, m_vMoveAxis, WVec2I32(e->pos().x(), e->pos().y()), m_mInvViewProj, vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return WEditorInput::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(WVec2I32(e->pos().x(), e->pos().y()), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return WEditorInput::WasExclusivelyHandled;
      }
    }


    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const WVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const WVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    vTranslate = vNewPos - m_vStartPosition;
  }
  else
  {
    const float fSpeed = m_fCameraSpeed * 0.01f;

    m_vTotalMouseDiff += WVec2((float)(CurMousePos.x - m_vLastMousePos.x), (float)(CurMousePos.y - m_vLastMousePos.y));
    const WVec3 vMouseDir = m_pCamera->GetDirRight() * m_vTotalMouseDiff.x + -m_pCamera->GetDirUp() * m_vTotalMouseDiff.y;

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate = m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
  {
    WSnapProvider::SnapTranslationInLocalSpace(mTrans.m_qRotation, vTranslate);
  }

  const WVec3 vLastPos = mTrans.m_vPosition;

  mTrans.m_vPosition = m_vStartPosition + vTranslate;

  m_vLastMoveDiff = mTrans.m_vPosition - vLastPos;

  SetTransformation(mTrans);

  // set statusbar message
  {
    const WVec3 diff = GetTransformation().m_qRotation.GetInverse() * GetTranslationResult();
    GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Translation: {}, {}, {}", WArgF(diff.x, 2), WArgF(diff.y, 2), WArgF(diff.z, 2)));
  }

  if (!m_vLastMoveDiff.IsZero())
  {
    WGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = WGizmoEvent::Type::Interaction;
    m_GizmoEvents.Broadcast(ev);
  }

  return WEditorInput::WasExclusivelyHandled;
}

void WTranslateGizmo::SetMovementMode(MovementMode mode)
{
  if (m_MovementMode == mode)
    return;

  m_MovementMode = mode;

  if (m_MovementMode == MovementMode::MouseDiff)
  {
    m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::WrapAtScreenBorders);
  }
}

void WTranslateGizmo::SetCameraSpeed(float fSpeed)
{
  m_fCameraSpeed = fSpeed;
}

void WTranslateGizmo::UpdateStatusBarText(WQtEngineDocumentWindow* pWindow)
{
  const WVec3 diff = WVec3::MakeZero();
  GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Translation: {}, {}, {}", WArgF(diff.x, 2), WArgF(diff.y, 2), WArgF(diff.z, 2)));
}

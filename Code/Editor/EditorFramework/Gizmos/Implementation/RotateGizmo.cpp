#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRotateGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WRotateGizmo::WRotateGizmo()
{
  const WColor colr = WColorScheme::LightUI(WColorScheme::Red);
  const WColor colg = WColorScheme::LightUI(WColorScheme::Green);
  const WColor colb = WColorScheme::LightUI(WColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneX.obj");
  m_hAxisY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneY.obj");
  m_hAxisZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WRotateGizmo::UpdateStatusBarText(WQtEngineDocumentWindow* pWindow)
{
  GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Rotation: {}", WAngle()));
}

void WRotateGizmo::EnableAxis(bool x, bool y, bool z)
{
  m_bEnableAxisX = x;
  m_bEnableAxisY = y;
  m_bEnableAxisZ = z;
}

void WRotateGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
}

void WRotateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible && m_bEnableAxisX);
  m_hAxisY.SetVisible(bVisible && m_bEnableAxisY);
  m_hAxisZ.SetVisible(bVisible && m_bEnableAxisZ);
}

void WRotateGizmo::OnTransformationChanged(const WTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
}

void WRotateGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(m_bEnableAxisX);
  m_hAxisY.SetVisible(m_bEnableAxisY);
  m_hAxisZ.SetVisible(m_bEnableAxisZ);
}

WEditorInput WRotateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;

  const WQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vRotationAxis = gizmoRot * WVec3(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vRotationAxis = gizmoRot * WVec3(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vRotationAxis = gizmoRot * WVec3(0, 0, 1);
  }
  else
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_Rotation = WAngle();

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_qStartRotation = GetTransformation().m_qRotation;

  WMat4 mView = m_pCamera->GetViewMatrix();
  WMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  WMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

  // compute screen space tangent for rotation
  {
    const WVec3 vAxisWS = m_vRotationAxis.GetNormalized();
    const WVec3 vMousePos(e->pos().x(), e->pos().y(), 0);
    const WVec3 vGizmoPosWS = GetTransformation().m_vPosition;

    WVec3 vPosOnNearPlane, vRayDir;
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vMousePos, vPosOnNearPlane, &vRayDir).IgnoreResult();

    WPlane plane;
    plane = WPlane::MakeFromNormalAndPoint(vAxisWS, vGizmoPosWS);

    WVec3 vPointOnGizmoWS;
    if (!plane.GetRayIntersection(vPosOnNearPlane, vRayDir, nullptr, &vPointOnGizmoWS))
    {
      // fallback at grazing angles, will result in fallback vDirWS during normalization
      vPointOnGizmoWS = vGizmoPosWS;
    }

    WVec3 vDirWS = vPointOnGizmoWS - vGizmoPosWS;
    vDirWS.NormalizeIfNotZero(WVec3(1, 0, 0)).IgnoreResult();

    WVec3 vTangentWS = vAxisWS.CrossRH(vDirWS);
    vTangentWS.Normalize();

    const WVec3 vTangentEndWS = vPointOnGizmoWS + vTangentWS;

    // compute the screen space position of the end point of the tangent vector, so that we can then compute the tangent in screen space
    WVec3 vTangentEndSS;
    WGraphicsUtils::ConvertWorldPosToScreenPos(mViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vTangentEndWS, vTangentEndSS).IgnoreResult();
    vTangentEndSS.z = 0;

    const WVec3 vTangentSS = vTangentEndSS - vMousePos;
    m_vScreenTangent.Set(vTangentSS.x, vTangentSS.y);
    m_vScreenTangent.NormalizeIfNotZero(WVec2(1, 0)).IgnoreResult();

    // because window coordinates are flipped along Y
    m_vScreenTangent.y = -m_vScreenTangent.y;
  }

  m_LastInteraction = WTime::Now();

  SetActiveInputContext(this);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WRotateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WRotateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const WVec2 vNewMousePos = WVec2(mousePosition.x(), mousePosition.y());
  WVec2 vDiff = vNewMousePos - WVec2(m_vLastMousePos.x, m_vLastMousePos.y);

  m_vLastMousePos = UpdateMouseMode(e);

  const float dv = m_vScreenTangent.Dot(vDiff);
  m_Rotation += WAngle::MakeFromDegree(dv);

  WAngle rot = m_Rotation;

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    WSnapProvider::SnapRotation(rot);

  m_qCurrentRotation = WQuat::MakeFromAxisAndAngle(m_vRotationAxis, rot);

  WTransform mTrans = GetTransformation();
  mTrans.m_qRotation = m_qCurrentRotation * m_qStartRotation;

  SetTransformation(mTrans);

  GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Rotation: {}", rot));

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConeAngleGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WConeAngleGizmo::WConeAngleGizmo()
{
  m_Angle = WAngle::MakeFromDegree(1.0f);
  m_fAngleScale = 1.0f;
  m_fRadius = 1.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeAngle.ConfigureHandle(this, WEngineGizmoHandleType::Cone, WColorLinearUB(200, 200, 0, 128), WGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WConeAngleGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeAngle);
}

void WConeAngleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeAngle.SetVisible(bVisible);
}

void WConeAngleGizmo::OnTransformationChanged(const WTransform& transform)
{
  WTransform t = transform;

  t.m_vScale *= WVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fRadius;
  m_hConeAngle.SetTransformation(t);
}

void WConeAngleGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeAngle.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WConeAngleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeAngle)
  {
    m_ManipulateMode = ManipulateMode::Angle;
  }
  else
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = WTime::Now();

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WConeAngleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WConeAngleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const WVec2I32 vNewMousePos = WVec2I32(mousePosition.x(), mousePosition.y());
  const WVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;
  const WAngle aSpeed = WAngle::MakeFromDegree(1.0f);

  {
    m_Angle += float(vDiff.x) * aSpeed;
    m_Angle -= float(vDiff.y) * aSpeed;

    m_Angle = WMath::Clamp(m_Angle, WAngle(), WAngle::MakeFromDegree(179.0f));

    m_fAngleScale = WMath::Tan(m_Angle * 0.5f);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WConeAngleGizmo::SetAngle(WAngle angle)
{
  m_Angle = angle;
  m_fAngleScale = WMath::Tan(m_Angle * 0.5f);

  // update the scale
  OnTransformationChanged(GetTransformation());
}

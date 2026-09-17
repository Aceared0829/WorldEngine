#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConeLengthGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WConeLengthGizmo::WConeLengthGizmo()
{
  m_fRadius = 1.0f;
  m_fRadiusScale = 0.1f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeRadius.ConfigureHandle(this, WEngineGizmoHandleType::Cone, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable | WGizmoFlags::OnTop); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WConeLengthGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeRadius);
}

void WConeLengthGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeRadius.SetVisible(bVisible);
}

void WConeLengthGizmo::OnTransformationChanged(const WTransform& transform)
{
  WTransform t = transform;
  t.m_vScale *= WVec3(1.0f, m_fRadiusScale, m_fRadiusScale) * m_fRadius;

  m_hConeRadius.SetTransformation(t);
}

void WConeLengthGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WConeLengthGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
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

WEditorInput WConeLengthGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WConeLengthGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = WMath::Max(0.0f, m_fRadius);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WConeLengthGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

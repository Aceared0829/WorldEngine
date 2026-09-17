#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSphereGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSphereGizmo::WSphereGizmo()
{
  m_bInnerEnabled = false;

  m_fRadiusInner = 1.0f;
  m_fRadiusOuter = 2.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hInnerSphere.ConfigureHandle(this, WEngineGizmoHandleType::Sphere, WColorLinearUB(200, 200, 0, 128), WGizmoFlags::OnTop | WGizmoFlags::Pickable); // this gizmo should be rendered very last so it is always on top
  m_hOuterSphere.ConfigureHandle(this, WEngineGizmoHandleType::Sphere, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WSphereGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hInnerSphere);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOuterSphere);
}

void WSphereGizmo::OnVisibleChanged(bool bVisible)
{
  m_hInnerSphere.SetVisible(bVisible && m_bInnerEnabled);
  m_hOuterSphere.SetVisible(bVisible);
}

void WSphereGizmo::OnTransformationChanged(const WTransform& transform)
{
  WTransform mScaleInner, mScaleOuter;
  mScaleInner.SetIdentity();
  mScaleOuter.SetIdentity();
  mScaleInner.m_vScale = WVec3(m_fRadiusInner);
  mScaleOuter.m_vScale = WVec3(m_fRadiusOuter);

  m_hInnerSphere.SetTransformation(transform * mScaleInner);
  m_hOuterSphere.SetTransformation(transform * mScaleOuter);
}

void WSphereGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hInnerSphere.SetVisible(m_bInnerEnabled);
  m_hOuterSphere.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WSphereGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hInnerSphere)
  {
    m_ManipulateMode = ManipulateMode::InnerSphere;
  }
  else if (m_pInteractionGizmoHandle == &m_hOuterSphere)
  {
    m_ManipulateMode = ManipulateMode::OuterSphere;
  }
  else
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // m_InnerSphere.SetVisible(false);
  // m_OuterSphere.SetVisible(false);

  // m_pInteractionGizmoHandle->SetVisible(true);

  m_LastInteraction = WTime::Now();

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WSphereGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WSphereGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  if (m_ManipulateMode == ManipulateMode::InnerSphere)
  {
    m_fRadiusInner += vDiff.x * fSpeed;
    m_fRadiusInner -= vDiff.y * fSpeed;

    m_fRadiusInner = WMath::Max(0.0f, m_fRadiusInner);

    m_fRadiusOuter = WMath::Max(m_fRadiusInner, m_fRadiusOuter);
  }
  else
  {
    m_fRadiusOuter += vDiff.x * fSpeed;
    m_fRadiusOuter -= vDiff.y * fSpeed;

    m_fRadiusOuter = WMath::Max(0.0f, m_fRadiusOuter);

    m_fRadiusInner = WMath::Min(m_fRadiusInner, m_fRadiusOuter);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WSphereGizmo::SetInnerSphere(bool bEnabled, float fRadius)
{
  m_fRadiusInner = fRadius;
  m_bInnerEnabled = bEnabled;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void WSphereGizmo::SetOuterSphere(float fRadius)
{
  m_fRadiusOuter = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

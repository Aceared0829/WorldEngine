#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCapsuleGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WCapsuleGizmo::WCapsuleGizmo()
{
  m_fLength = 1.0f;
  m_fRadius = 0.25f;

  m_ManipulateMode = ManipulateMode::None;

  m_hRadius.ConfigureHandle(this, WEngineGizmoHandleType::CylinderZ, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);
  m_hLengthTop.ConfigureHandle(this, WEngineGizmoHandleType::HalfSphereZ, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);
  m_hLengthBottom.ConfigureHandle(this, WEngineGizmoHandleType::HalfSphereZ, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WCapsuleGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthBottom);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hRadius);
}

void WCapsuleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hLengthTop.SetVisible(bVisible);
  m_hLengthBottom.SetVisible(bVisible);
  m_hRadius.SetVisible(bVisible);
}

void WCapsuleGizmo::OnTransformationChanged(const WTransform& transform)
{
  {
    WTransform mScaleCylinder;
    mScaleCylinder.SetIdentity();
    mScaleCylinder.m_vScale = WVec3(m_fRadius, m_fRadius, m_fLength);

    m_hRadius.SetTransformation(transform * mScaleCylinder);
  }

  {
    WTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, m_fLength * 0.5f);
    m_hLengthTop.SetTransformation(transform * mScaleSpheres);
  }

  {
    WTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius, -m_fRadius, -m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, -m_fLength * 0.5f);
    m_hLengthBottom.SetTransformation(transform * mScaleSpheres);
  }
}

void WCapsuleGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hLengthTop.SetVisible(true);
  m_hLengthBottom.SetVisible(true);
  m_hRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WCapsuleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else if (m_pInteractionGizmoHandle == &m_hLengthTop || m_pInteractionGizmoHandle == &m_hLengthBottom)
  {
    m_ManipulateMode = ManipulateMode::Length;
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

WEditorInput WCapsuleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WCapsuleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  QPoint mousePosition = e->globalPosition().toPoint();

  const WVec2I32 vNewMousePos = WVec2I32(mousePosition.x(), mousePosition.y());
  const WVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = WMath::Max(0.0f, m_fRadius);
  }
  else
  {
    m_fLength += vDiff.x * fSpeed;
    m_fLength -= vDiff.y * fSpeed;

    m_fLength = WMath::Max(0.0f, m_fLength);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WCapsuleGizmo::SetLength(float fRadius)
{
  m_fLength = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void WCapsuleGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

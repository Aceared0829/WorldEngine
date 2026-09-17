#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/BoxGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoxGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WBoxGizmo::WBoxGizmo()
{
  m_vSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hCorners.ConfigureHandle(this, WEngineGizmoHandleType::BoxCorners, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].ConfigureHandle(this, WEngineGizmoHandleType::BoxEdges, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);
    m_Faces[i].ConfigureHandle(this, WEngineGizmoHandleType::BoxFaces, WColorLinearUB(200, 200, 200, 128), WGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WBoxGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hCorners);

  for (int i = 0; i < 3; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Edges[i]);
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Faces[i]);
  }
}

void WBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hCorners.SetVisible(bVisible);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].SetVisible(bVisible);
    m_Faces[i].SetVisible(bVisible);
  }
}

void WBoxGizmo::OnTransformationChanged(const WTransform& transform)
{
  WMat4 scale, rot;
  scale = WMat4::MakeScaling(m_vSize);
  scale = transform.GetAsMat4() * scale;

  m_hCorners.SetTransformation(scale);

  rot = WMat4::MakeRotationX(WAngle::MakeFromDegree(90));
  m_Edges[0].SetTransformation(scale * rot);

  rot = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));
  m_Faces[0].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Edges[1].SetTransformation(scale * rot);

  rot = WMat4::MakeRotationX(WAngle::MakeFromDegree(90));
  m_Faces[1].SetTransformation(scale * rot);

  rot = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90));
  m_Edges[2].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Faces[2].SetTransformation(scale * rot);
}

void WBoxGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

WEditorInput WBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hCorners)
  {
    m_ManipulateMode = ManipulateMode::Uniform;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[0])
  {
    m_ManipulateMode = ManipulateMode::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[1])
  {
    m_ManipulateMode = ManipulateMode::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[2])
  {
    m_ManipulateMode = ManipulateMode::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[0])
  {
    m_ManipulateMode = ManipulateMode::PlaneXY;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[1])
  {
    m_ManipulateMode = ManipulateMode::PlaneXZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[2])
  {
    m_ManipulateMode = ManipulateMode::PlaneYZ;
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

WEditorInput WBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
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
  float fChange = 0.0f;

  {
    fChange += vDiff.x * fSpeed;
    fChange -= vDiff.y * fSpeed;
  }

  WVec3 vChange(0);

  if (m_ManipulateMode == ManipulateMode::Uniform)
    vChange.Set(fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneXY)
    vChange.Set(fChange, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::PlaneXZ)
    vChange.Set(fChange, 0, fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneYZ)
    vChange.Set(0, fChange, fChange);
  if (m_ManipulateMode == ManipulateMode::AxisX)
    vChange.Set(fChange, 0, 0);
  if (m_ManipulateMode == ManipulateMode::AxisY)
    vChange.Set(0, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::AxisZ)
    vChange.Set(0, 0, fChange);

  m_vSize += vChange;
  m_vSize.x = WMath::Max(m_vSize.x, 0.0f);
  m_vSize.y = WMath::Max(m_vSize.y, 0.0f);
  m_vSize.z = WMath::Max(m_vSize.z, 0.0f);

  // update the scale
  OnTransformationChanged(GetTransformation());

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

void WBoxGizmo::SetSize(const WVec3& vSize)
{
  m_vSize = vSize;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

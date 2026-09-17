#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WClickGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WClickGizmo::WClickGizmo()
{
  m_hShape.ConfigureHandle(this, WEngineGizmoHandleType::Sphere, WColor::White, WGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WClickGizmo::SetColor(const WColor& color)
{
  m_hShape.SetColor(color);
}

void WClickGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hShape);
}

void WClickGizmo::OnVisibleChanged(bool bVisible)
{
  m_hShape.SetVisible(bVisible);
}

void WClickGizmo::OnTransformationChanged(const WTransform& transform)
{
  m_hShape.SetTransformation(transform);
}

void WClickGizmo::DoFocusLost(bool bCancel)
{
  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);
}

WEditorInput WClickGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle != &m_hShape)
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetActiveInputContext(this);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WClickGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  FocusLost(false);

  SetActiveInputContext(nullptr);

  return WEditorInput::WasExclusivelyHandled;
}

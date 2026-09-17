#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDragToPositionGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WDragToPositionGizmo::WDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  // TODO: adjust colors for +/- axis
  const WColor colr1 = WColorGammaUB(206, 0, 46);
  const WColor colr2 = WColorGammaUB(206, 0, 46);
  const WColor colg1 = WColorGammaUB(101, 206, 0);
  const WColor colg2 = WColorGammaUB(101, 206, 0);
  const WColor colb1 = WColorGammaUB(0, 125, 206);
  const WColor colb2 = WColorGammaUB(0, 125, 206);
  const WColor coly = WColorGammaUB(128, 128, 0);

  m_hBobble.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, coly, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragCenter.obj");
  m_hAlignPX.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr1, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowPX.obj");
  m_hAlignNX.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr2, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowNX.obj");
  m_hAlignPY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg1, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowPY.obj");
  m_hAlignNY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg2, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowNY.obj");
  m_hAlignPZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb1, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowPZ.obj");
  m_hAlignNZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb2, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/DragArrowNZ.obj");

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WDragToPositionGizmo::UpdateStatusBarText(WQtEngineDocumentWindow* pWindow)
{
  if (m_pInteractionGizmoHandle != nullptr)
  {
    if (m_pInteractionGizmoHandle == &m_hBobble)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: Center"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: +X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: -X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: +Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: -Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: +Z"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position: -Z"));
  }
  else
  {
    GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Drag to Position"));
  }
}

void WDragToPositionGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBobble);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNZ);
}

void WDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_hBobble.SetVisible(bVisible);
  m_hAlignPX.SetVisible(bVisible);
  m_hAlignNX.SetVisible(bVisible);
  m_hAlignPY.SetVisible(bVisible);
  m_hAlignNY.SetVisible(bVisible);
  m_hAlignPZ.SetVisible(bVisible);
  m_hAlignNZ.SetVisible(bVisible);
}

void WDragToPositionGizmo::OnTransformationChanged(const WTransform& transform)
{
  m_hBobble.SetTransformation(transform);
  m_hAlignPX.SetTransformation(transform);
  m_hAlignNX.SetTransformation(transform);
  m_hAlignPY.SetTransformation(transform);
  m_hAlignNY.SetTransformation(transform);
  m_hAlignPZ.SetTransformation(transform);
  m_hAlignNZ.SetTransformation(transform);
}

void WDragToPositionGizmo::DoFocusLost(bool bCancel)
{
  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hBobble.SetVisible(true);
  m_hAlignPX.SetVisible(true);
  m_hAlignNX.SetVisible(true);
  m_hAlignPY.SetVisible(true);
  m_hAlignNY.SetVisible(true);
  m_hAlignPZ.SetVisible(true);
  m_hAlignNZ.SetVisible(true);

  m_pInteractionGizmoHandle = nullptr;
}

WEditorInput WDragToPositionGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // The gizmo is actually "hidden" somewhere else during dragging,
  // because it musn't be rendered into the picking buffer, to avoid picking against the gizmo
  // m_Bobble.SetVisible(false);
  // m_AlignPX.SetVisible(false);
  // m_AlignNX.SetVisible(false);
  // m_AlignPY.SetVisible(false);
  // m_AlignNY.SetVisible(false);
  // m_AlignPZ.SetVisible(false);
  // m_AlignNZ.SetVisible(false);
  // m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_qStartOrientation = GetTransformation().m_qRotation;

  m_LastInteraction = WTime::Now();

  SetActiveInputContext(this);

  UpdateStatusBarText(nullptr);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WDragToPositionGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return WEditorInput::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return WEditorInput::WasExclusivelyHandled;

  WVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    WSnapProvider::SnapTranslation(vSnappedPosition);

  WTransform mTrans = GetTransformation();
  mTrans.m_vPosition = vSnappedPosition;

  WQuat rot;
  WVec3 alignAxis, orthoAxis;

  if (m_pInteractionGizmoHandle == &m_hAlignPX)
  {
    alignAxis.Set(1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNX)
  {
    alignAxis.Set(-1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPY)
  {
    alignAxis.Set(0, 1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNY)
  {
    alignAxis.Set(0, -1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
  {
    alignAxis.Set(0, 0, 1);
    orthoAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
  {
    alignAxis.Set(0, 0, -1);
    orthoAxis.Set(1, 0, 0);
  }
  else
  {
    m_bModifiesRotation = false;
    rot.SetIdentity();
  }

  if (m_pInteractionGizmoHandle != &m_hBobble)
  {
    m_bModifiesRotation = true;

    alignAxis = m_qStartOrientation * alignAxis;
    alignAxis.Normalize();

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > WAngle::MakeFromDegree(179))
    {
      rot = WQuat::MakeFromAxisAndAngle(m_qStartOrientation * orthoAxis, WAngle::MakeFromDegree(180));
    }
    else
    {
      rot = WQuat::MakeShortestRotation(alignAxis, res.m_vPickedNormal);
    }
  }

  mTrans.m_qRotation = rot * m_qStartOrientation;
  SetTransformation(mTrans);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

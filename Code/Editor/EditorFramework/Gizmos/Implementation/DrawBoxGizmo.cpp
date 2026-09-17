#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDrawBoxGizmo, 1, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

WDrawBoxGizmo::WDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_vLastStartPoint.SetZero();
  m_hBox.ConfigureHandle(this, WEngineGizmoHandleType::LineBox, WColorLinearUB(255, 100, 0), WGizmoFlags::ShowInOrtho);

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

WDrawBoxGizmo::~WDrawBoxGizmo() = default;

void WDrawBoxGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBox);
}

void WDrawBoxGizmo::OnVisibleChanged(bool bVisible) {}

void WDrawBoxGizmo::OnTransformationChanged(const WTransform& transform) {}

void WDrawBoxGizmo::DoFocusLost(bool bCancel)
{
  WViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
  UpdateBox();

  if (IsActiveInputContext())
    SetActiveInputContext(nullptr);
}

bool WDrawBoxGizmo::PickPosition(QMouseEvent* e)
{
  const QPoint mousePos = GetOwnerWindow()->mapFromGlobal(QCursor::pos());

  const WObjectPickingResult& res = GetOwnerView()->PickObject(mousePos.x(), mousePos.y());

  m_vUpAxis = GetOwnerView()->GetFallbackPickingPlane().m_vNormal;
  m_vUpAxis.x = WMath::Abs(m_vUpAxis.x);
  m_vUpAxis.y = WMath::Abs(m_vUpAxis.y);
  m_vUpAxis.z = WMath::Abs(m_vUpAxis.z);

  if (res.m_PickedObject.IsValid() && !e->modifiers().testFlag(Qt::ShiftModifier))
  {
    m_vCurrentPosition = res.m_vPickedPosition;
  }
  else
  {
    if (GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), GetOwnerView()->GetFallbackPickingPlane(m_vLastStartPoint), m_vCurrentPosition).Failed())
    {
      return false;
    }
  }

  WSnapProvider::SnapTranslation(m_vCurrentPosition);
  return true;
}

WEditorInput WDrawBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (e->buttons() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier)
  {
    if (m_ManipulateMode == ManipulateMode::None)
    {
      if (!PickPosition(e))
      {
        return WEditorInput::WasExclusivelyHandled; // failed to pick anything
      }

      m_vLastStartPoint = m_vCurrentPosition;
      SwitchMode(false);
      return WEditorInput::WasExclusivelyHandled;
    }
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WDrawBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::LeftButton)
  {
    if (m_ManipulateMode == ManipulateMode::DrawBase || m_ManipulateMode == ManipulateMode::DrawHeight)
    {
      SwitchMode(m_vFirstCorner == m_vSecondCorner);
      return WEditorInput::WasExclusivelyHandled;
    }
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WDrawBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  UpdateGrid(e);

  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    const QPoint mousePosition = e->globalPosition().toPoint();
    const WVec2I32 vMouseMove = WVec2I32(mousePosition.x(), mousePosition.y()) - m_vLastMousePos;
    m_iHeightChange -= vMouseMove.y;

    m_vLastMousePos = UpdateMouseMode(e);
  }
  else
  {
    WPlane plane;
    plane = WPlane::MakeFromNormalAndPoint(m_vUpAxis, m_vFirstCorner);

    GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), plane, m_vCurrentPosition).IgnoreResult();

    WSnapProvider::SnapTranslation(m_vCurrentPosition);
  }

  UpdateBox();

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WDrawBoxGizmo::DoKeyPressEvent(QKeyEvent* e)
{
  // is the gizmo in general visible == is it active
  if (!IsVisible())
    return WEditorInput::MayBeHandledByOthers;

  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  if (e->key() == Qt::Key_Escape)
  {
    if (m_ManipulateMode != ManipulateMode::None)
    {
      SwitchMode(true);
      return WEditorInput::WasExclusivelyHandled;
    }
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WDrawBoxGizmo::DoKeyReleaseEvent(QKeyEvent* e)
{
  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  return WEditorInput::MayBeHandledByOthers;
}

void WDrawBoxGizmo::SwitchMode(bool bCancel)
{
  WGizmoEvent e;
  e.m_pGizmo = this;

  if (bCancel)
  {
    FocusLost(true);

    e.m_Type = WGizmoEvent::Type::CancelInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::None)
  {
    m_ManipulateMode = ManipulateMode::DrawBase;
    m_vFirstCorner = m_vCurrentPosition;
    m_vSecondCorner = m_vFirstCorner;

    SetActiveInputContext(this);
    UpdateBox();

    e.m_Type = WGizmoEvent::Type::BeginInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_ManipulateMode = ManipulateMode::DrawHeight;
    m_iHeightChange = 0;
    m_fOriginalBoxHeight = m_fBoxHeight;
    m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    UpdateBox();
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    e.m_Type = WGizmoEvent::Type::EndInteractions;
    m_GizmoEvents.Broadcast(e);

    UpdateBox();
    FocusLost(false);
    return;
  }
}

void WDrawBoxGizmo::UpdateBox()
{
  UpdateStatusBarText(GetOwnerWindow());

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_vSecondCorner = m_vCurrentPosition;
    m_vSecondCorner.x = WMath::Lerp(m_vSecondCorner.x, m_vFirstCorner.x, m_vUpAxis.x);
    m_vSecondCorner.y = WMath::Lerp(m_vSecondCorner.y, m_vFirstCorner.y, m_vUpAxis.y);
    m_vSecondCorner.z = WMath::Lerp(m_vSecondCorner.z, m_vFirstCorner.z, m_vUpAxis.z);
  }

  if (m_ManipulateMode == ManipulateMode::None || m_vFirstCorner == m_vSecondCorner)
  {
    m_hBox.SetTransformation(WTransform(WVec3(0), WQuat::MakeIdentity(), WVec3(0)));
    m_hBox.SetVisible(false);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    m_fBoxHeight = m_fOriginalBoxHeight + ((float)m_iHeightChange * 0.1f * WSnapProvider::GetTranslationSnapValue());
    WVec3 snapDummy(m_fBoxHeight);
    WSnapProvider::SnapTranslation(snapDummy);
    m_fBoxHeight = m_vUpAxis.Dot(snapDummy);
  }

  WVec3 vCenter = WMath::Lerp(m_vFirstCorner, m_vSecondCorner, 0.5f);
  vCenter.x += m_fBoxHeight * 0.5f * m_vUpAxis.x;
  vCenter.y += m_fBoxHeight * 0.5f * m_vUpAxis.y;
  vCenter.z += m_fBoxHeight * 0.5f * m_vUpAxis.z;

  WVec3 vSize;

  if (m_vUpAxis.z != 0)
  {
    vSize.x = WMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.y = WMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.z = m_fBoxHeight;
  }
  else if (m_vUpAxis.x != 0)
  {
    vSize.z = WMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = WMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.x = m_fBoxHeight;
  }
  else if (m_vUpAxis.y != 0)
  {
    vSize.x = WMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.z = WMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = m_fBoxHeight;
  }

  m_hBox.SetTransformation(WTransform(vCenter, WQuat::MakeIdentity(), vSize));
  m_hBox.SetVisible(true);
}

void WDrawBoxGizmo::DisableGrid(bool bControlPressed)
{
  if (!bControlPressed)
  {
    m_bDisplayGrid = false;
  }
}

void WDrawBoxGizmo::UpdateGrid(QMouseEvent* e)
{
  m_bDisplayGrid = false;

  if (m_ManipulateMode == ManipulateMode::None && e->modifiers().testFlag(Qt::ControlModifier))
  {
    if (e != nullptr && PickPosition(e))
    {
      m_vFirstCorner = m_vCurrentPosition;
      m_bDisplayGrid = true;
    }
  }
}

void WDrawBoxGizmo::GetResult(WVec3& out_vOrigin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const
{
  out_vOrigin = m_vFirstCorner;

  float fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
  float fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
  float fBoxZ = m_fBoxHeight;

  if (m_vUpAxis.x != 0)
  {
    fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxX = m_fBoxHeight;
  }

  if (m_vUpAxis.y != 0)
  {
    fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxY = m_fBoxHeight;
  }

  if (fBoxX > 0)
  {
    out_fSizeNegX = 0;
    out_fSizePosX = fBoxX;
  }
  else
  {
    out_fSizeNegX = -fBoxX;
    out_fSizePosX = 0;
  }

  if (fBoxY > 0)
  {
    out_fSizeNegY = 0;
    out_fSizePosY = fBoxY;
  }
  else
  {
    out_fSizeNegY = -fBoxY;
    out_fSizePosY = 0;
  }

  if (fBoxZ > 0)
  {
    out_fSizeNegZ = 0;
    out_fSizePosZ = fBoxZ;
  }
  else
  {
    out_fSizeNegZ = -fBoxZ;
    out_fSizePosZ = 0;
  }
}

void WDrawBoxGizmo::UpdateStatusBarText(WQtEngineDocumentWindow* pWindow)
{
  switch (m_ManipulateMode)
  {
    case ManipulateMode::None:
    {
      pWindow->SetPermanentStatusBarMsg("Greyboxing: Hold CTRL and click-drag to draw a box. Hold SHIFT to reuse the previous plane height.");
      break;
    }

    case ManipulateMode::DrawBase:
    {
      WVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x = WMath::Abs(diff.x);
      diff.y = WMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(WFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Release the mouse to finish the base. ESC to cancel.", WArgF(diff.y, 2, false, 2), WArgF(diff.x, 2, false, 2), WArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }

    case ManipulateMode::DrawHeight:
    {
      WVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x = WMath::Abs(diff.x);
      diff.y = WMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(WFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Draw up/down to specify the box height. Click to finish, ESC to cancel.", WArgF(diff.y, 2, false, 2), WArgF(diff.x, 2, false, 2), WArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }
  }
}

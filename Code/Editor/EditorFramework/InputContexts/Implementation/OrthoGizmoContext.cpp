#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOrthoGizmoContext, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WOrthoGizmoContext::WOrthoGizmoContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}


void WOrthoGizmoContext::FocusLost(bool bCancel)
{
  WGizmoEvent e;
  e.m_pGizmo = this;
  e.m_Type = bCancel ? WGizmoEvent::Type::CancelInteractions : WGizmoEvent::Type::EndInteractions;

  m_GizmoEvents.Broadcast(e);


  m_bCanInteract = false;
  SetActiveInputContext(nullptr);

  WEditorInputContext::FocusLost(bCancel);
}

WEditorInput WOrthoGizmoContext::DoMousePressEvent(QMouseEvent* e)
{
  if (!IsViewInOrthoMode())
    return WEditorInput::MayBeHandledByOthers;
  if (GetOwnerWindow()->GetDocument()->GetSelectionManager()->IsSelectionEmpty())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bCanInteract = true;
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WOrthoGizmoContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
  {
    m_bCanInteract = false;
    return WEditorInput::MayBeHandledByOthers;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    FocusLost(false);
    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WOrthoGizmoContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!e->buttons().testFlag(Qt::MouseButton::LeftButton))
  {
    m_bCanInteract = false;
    return WEditorInput::MayBeHandledByOthers;
  }

  if (IsActiveInputContext())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == WCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == WCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    const WVec3 vLastTranslationResult = m_vTranslationResult;

    const QPoint mousePosition = e->globalPosition().toPoint();

    const WVec2I32 diff = WVec2I32(mousePosition.x(), mousePosition.y()) - m_vLastMousePos;

    m_vUnsnappedTranslationResult += m_pCamera->GetDirRight() * (float)diff.x * fDistPerPixel;
    m_vUnsnappedTranslationResult -= m_pCamera->GetDirUp() * (float)diff.y * fDistPerPixel;

    m_vTranslationResult = m_vUnsnappedTranslationResult;

    // disable snapping when SHIFT is pressed
    if (!e->modifiers().testFlag(Qt::ShiftModifier))
      WSnapProvider::SnapTranslation(m_vTranslationResult);

    m_vTranslationDiff = m_vTranslationResult - vLastTranslationResult;

    m_UnsnappedRotationResult += WAngle::MakeFromDegree(-diff.x);

    WAngle snappedRotation = m_UnsnappedRotationResult;

    // disable snapping when SHIFT is pressed
    if (!e->modifiers().testFlag(Qt::ShiftModifier))
      WSnapProvider::SnapRotation(snappedRotation);

    m_qRotationResult = WQuat::MakeFromAxisAndAngle(m_pCamera->GetDirForwards(), snappedRotation);

    {
      m_fScaleMouseMove += diff.x;
      m_fUnsnappedScalingResult = 1.0f;

      const float fScaleSpeed = 0.01f;

      if (m_fScaleMouseMove > 0.0f)
        m_fUnsnappedScalingResult = 1.0f + m_fScaleMouseMove * fScaleSpeed;
      if (m_fScaleMouseMove < 0.0f)
        m_fUnsnappedScalingResult = 1.0f / (1.0f - m_fScaleMouseMove * fScaleSpeed);

      m_fScalingResult = m_fUnsnappedScalingResult;

      // disable snapping when SHIFT is pressed
      if (!e->modifiers().testFlag(Qt::ShiftModifier))
        WSnapProvider::SnapScale(m_fScalingResult);
    }

    m_vLastMousePos = UpdateMouseMode(e);

    WGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = WGizmoEvent::Type::Interaction;

    m_GizmoEvents.Broadcast(ev);

    return WEditorInput::WasExclusivelyHandled;
  }

  if (m_bCanInteract)
  {
    m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::WrapAtScreenBorders);
    m_vTranslationResult.SetZero();
    m_vUnsnappedTranslationResult.SetZero();
    m_qRotationResult.SetIdentity();
    m_UnsnappedRotationResult = WAngle::MakeFromRadian(0.0f);
    m_fScalingResult = 1.0f;
    m_fUnsnappedScalingResult = 1.0f;
    m_fScaleMouseMove = 0.0f;

    m_bCanInteract = false;
    SetActiveInputContext(this);

    WGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = WGizmoEvent::Type::BeginInteractions;

    m_GizmoEvents.Broadcast(ev);
    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;
}

bool WOrthoGizmoContext::IsViewInOrthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective);
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScaleGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WScaleGizmo::WScaleGizmo()
{
  const WColor colr = WColorScheme::LightUI(WColorScheme::Red);
  const WColor colg = WColorScheme::LightUI(WColorScheme::Green);
  const WColor colb = WColorScheme::LightUI(WColorScheme::Blue);
  const WColor coly = WColorScheme::LightUI(WColorScheme::Gray);

  m_hAxisX.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colr, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
  m_hAxisY.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colg, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, colb, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowZ.obj");
  m_hAxisXYZ.ConfigureHandle(this, WEngineGizmoHandleType::FromFile, coly, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable, "Editor/Meshes/ScaleXYZ.obj");

  SetVisible(false);
  SetTransformation(WTransform::MakeIdentity());
}

void WScaleGizmo::UpdateStatusBarText(WQtEngineDocumentWindow* pWindow)
{
  const WVec3 scale(1.0f);
  GetOwnerWindow()->SetPermanentStatusBarMsg(WFmt("Scale: {}, {}, {}", WArgF(scale.x, 2), WArgF(scale.y, 2), WArgF(scale.z, 2)));
}

void WScaleGizmo::EnableAxis(bool x, bool y, bool z, bool bXyz)
{
  m_bEnableAxisX = x;
  m_bEnableAxisY = y;
  m_bEnableAxisZ = z;
  m_bEnableAxisXYZ = bXyz;
}

void WScaleGizmo::OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisXYZ);
}

void WScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible && m_bEnableAxisX);
  m_hAxisY.SetVisible(bVisible && m_bEnableAxisY);
  m_hAxisZ.SetVisible(bVisible && m_bEnableAxisZ);
  m_hAxisXYZ.SetVisible(bVisible && m_bEnableAxisXYZ);
}

void WScaleGizmo::OnTransformationChanged(const WTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hAxisXYZ.SetTransformation(transform);
}

void WScaleGizmo::DoFocusLost(bool bCancel)
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
  m_hAxisXYZ.SetVisible(m_bEnableAxisXYZ);
}

WEditorInput WScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return WEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis.Set(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisXYZ)
  {
    m_vMoveAxis.Set(1, 1, 1);
  }
  else
    return WEditorInput::MayBeHandledByOthers;

  WViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  WMat4 mView = m_pCamera->GetViewMatrix();
  WMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  WMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

  m_LastInteraction = WTime::Now();

  SetActiveInputContext(this);

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WScaleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return WEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  const WTime tNow = WTime::Now();

  if (tNow - m_LastInteraction < WTime::MakeFromSeconds(1.0 / 25.0))
    return WEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const WVec2I32 vNewMousePos = WVec2I32(mousePosition.x(), mousePosition.y());
  WVec2I32 vDiff = (vNewMousePos - m_vLastMousePos);

  m_vLastMousePos = UpdateMouseMode(e);

  m_vScaleMouseMove += m_vMoveAxis * (float)vDiff.x;
  m_vScaleMouseMove -= m_vMoveAxis * (float)vDiff.y;

  m_vScalingResult.Set(1.0f);

  const float fScaleSpeed = 0.01f;

  if (m_vScaleMouseMove.x > 0.0f)
    m_vScalingResult.x = 1.0f + m_vScaleMouseMove.x * fScaleSpeed;
  if (m_vScaleMouseMove.x < 0.0f)
    m_vScalingResult.x = 1.0f / (1.0f - m_vScaleMouseMove.x * fScaleSpeed);

  if (m_vScaleMouseMove.y > 0.0f)
    m_vScalingResult.y = 1.0f + m_vScaleMouseMove.y * fScaleSpeed;
  if (m_vScaleMouseMove.y < 0.0f)
    m_vScalingResult.y = 1.0f / (1.0f - m_vScaleMouseMove.y * fScaleSpeed);

  if (m_vScaleMouseMove.z > 0.0f)
    m_vScalingResult.z = 1.0f + m_vScaleMouseMove.z * fScaleSpeed;
  if (m_vScaleMouseMove.z < 0.0f)
    m_vScalingResult.z = 1.0f / (1.0f - m_vScaleMouseMove.z * fScaleSpeed);

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    WSnapProvider::SnapScale(m_vScalingResult);

  GetOwnerWindow()->SetPermanentStatusBarMsg(
    WFmt("Scale: {}, {}, {}", WArgF(m_vScalingResult.x, 2), WArgF(m_vScalingResult.y, 2), WArgF(m_vScalingResult.z, 2)));

  WGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = WGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return WEditorInput::WasExclusivelyHandled;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WManipulatorScaleGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WManipulatorScaleGizmo::WManipulatorScaleGizmo()
{
  const WColor colr = WColorScheme::LightUI(WColorScheme::Red);
  const WColor colg = WColorScheme::LightUI(WColorScheme::Green);
  const WColor colb = WColorScheme::LightUI(WColorScheme::Blue);

  // Overwrite axis to be boxes.
  m_hAxisX.ConfigureHandle(this, WEngineGizmoHandleType::Box, colr, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable);
  m_hAxisY.ConfigureHandle(this, WEngineGizmoHandleType::Box, colg, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable);
  m_hAxisZ.ConfigureHandle(this, WEngineGizmoHandleType::Box, colb, WGizmoFlags::ConstantSize | WGizmoFlags::Pickable);
}

void WManipulatorScaleGizmo::OnTransformationChanged(const WTransform& transform)
{
  const float fOffset = 0.8f;
  WTransform t;
  t.SetIdentity();
  t.m_vPosition = WVec3(fOffset, 0, 0);
  t.m_vScale = WVec3(0.2f);

  m_hAxisX.SetTransformation(transform * t);

  t.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(90));
  t.m_vPosition = WVec3(0, fOffset, 0);
  m_hAxisY.SetTransformation(transform * t);

  t.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(-90));
  t.m_vPosition = WVec3(0, 0, fOffset);
  m_hAxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = WVec3(0.3f);
  m_hAxisXYZ.SetTransformation(transform * t);
}

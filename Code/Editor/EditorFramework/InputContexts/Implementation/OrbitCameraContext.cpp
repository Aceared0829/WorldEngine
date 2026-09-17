#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

WOrbitCameraContext::WOrbitCameraContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  m_Volume = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3::MakeZero());
  m_pCamera = nullptr;

  m_LastUpdate = WTime::Now();

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void WOrbitCameraContext::SetCamera(WCamera* pCamera)
{
  m_pCamera = pCamera;
}

WCamera* WOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}


void WOrbitCameraContext::SetDefaultCameraRelative(const WVec3& vDirection, float fDistanceScale)
{
  m_bFixedDefaultCamera = false;

  m_vDefaultCamera = vDirection;
  m_vDefaultCamera.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();
  m_vDefaultCamera *= WMath::Max(0.01f, fDistanceScale);
}

void WOrbitCameraContext::SetDefaultCameraFixed(const WVec3& vPosition)
{
  m_bFixedDefaultCamera = true;
  m_vDefaultCamera = vPosition;
}

void WOrbitCameraContext::MoveCameraToDefaultPosition()
{
  if (!m_pCamera)
    return;

  const WVec3 vCenterPos = m_Volume.GetCenter();
  WVec3 vCamPos = m_vDefaultCamera;

  if (!m_bFixedDefaultCamera)
  {
    const WVec3 ext = m_Volume.GetHalfExtents();

    vCamPos = vCenterPos + m_vDefaultCamera * WMath::Max(0.1f, WMath::Max(ext.x, ext.y, ext.z));
  }

  m_pCamera->LookAt(vCamPos, vCenterPos, WVec3(0, 0, 1));
}

void WOrbitCameraContext::SetOrbitVolume(const WVec3& vCenterPos, const WVec3& vHalfBoxSize)
{
  bool bSetCamLookAt = false;

  if (m_Volume.GetHalfExtents().IsZero() && !vHalfBoxSize.IsZero())
  {
    bSetCamLookAt = true;
  }

  m_Volume = WBoundingBox::MakeFromCenterAndHalfExtents(vCenterPos, vHalfBoxSize);

  if (bSetCamLookAt)
  {
    MoveCameraToDefaultPosition();
  }
}

void WOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  m_bRun = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveLeft = false;
  m_bMoveRight = false;
  m_bMoveUp = false;
  m_bMoveDown = false;

  ResetCursor();
}

WEditorInput WOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return WEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return WEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_Mode = Mode::Orbit;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_Mode = Mode::Free;
      goto activate;
    }
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    return WEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;

activate:
{
  m_vLastMousePos = SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  MakeActiveInputContext();
  return WEditorInput::WasExclusivelyHandled;
}
}

WEditorInput WOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return WEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return WEditorInput::MayBeHandledByOthers;


  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Free;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode = Mode::Off;
    m_bRun = false;
    m_bMoveForwards = false;
    m_bMoveBackwards = false;
    m_bMoveLeft = false;
    m_bMoveRight = false;
    m_bMoveUp = false;
    m_bMoveDown = false;
    ResetCursor();
  }

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return WEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return WEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return WEditorInput::MayBeHandledByOthers;

  const QSize viewSize = GetOwnerView()->size();

  const WEditorPreferencesUser* pEditorPref = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  const WVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const WVec2I32 mouseDiff = CurMousePos - m_vLastMousePos;
  m_vLastMousePos = UpdateMouseMode(e);

  WVec2 diffNorm = WVec2(mouseDiff.x, mouseDiff.y);

  switch (m_pCamera->GetCameraMode())
  {
    case WCameraMode::PerspectiveFixedFovX:
    case WCameraMode::OrthoFixedWidth:
      diffNorm /= (float)viewSize.width();
      break;
    case WCameraMode::PerspectiveFixedFovY:
    case WCameraMode::OrthoFixedHeight:
      diffNorm /= (float)viewSize.height();
      break;

    default:
      break;
  }

  SetCurrentMouseMode();

  const float fMouseRotationSpeed = 2.0f * pEditorPref->m_fCameraRotationSpeed;

  const WVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float fMaxExtent = WMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float fBoost = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 5.0f : 1.0f;

  if (m_Mode == Mode::Orbit)
  {
    const float fMoveRight = diffNorm.x;
    const float fMoveUp = -diffNorm.y;

    const WVec3 vOrbitPoint = m_Volume.GetCenter();

    const float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    if (fDistance > 0.01f)
    {
      // first force the camera to rotate towards the orbit point
      // this way the camera position doesn't jump around
      m_pCamera->LookAt(m_pCamera->GetCenterPosition(), vOrbitPoint, WVec3(0.0f, 0.0f, 1.0f));
    }

    // then rotate the camera, and adjust its position to again point at the orbit point

    m_pCamera->RotateLocally(WAngle::MakeFromRadian(0.0f), WAngle::MakeFromRadian(fMoveUp), WAngle::MakeFromRadian(0.0f));
    m_pCamera->RotateGlobally(WAngle::MakeFromRadian(0.0f), WAngle::MakeFromRadian(0.0f), WAngle::MakeFromRadian(fMoveRight));

    WVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, WVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::Free)
  {
    const float fAspectRatio = (float)viewSize.width() / (float)viewSize.height();
    const WAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const WAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseRotateSensitivityX = fFovX.GetRadian() * fMouseRotationSpeed;
    const float fMouseRotateSensitivityY = fFovY.GetRadian() * fMouseRotationSpeed;

    float fRotateHorizontal = diffNorm.x * fMouseRotateSensitivityX;
    float fRotateVertical = -diffNorm.y * fMouseRotateSensitivityY;

    m_pCamera->RotateLocally(WAngle::MakeFromRadian(0), WAngle::MakeFromRadian(fRotateVertical), WAngle::MakeFromRadian(0));
    m_pCamera->RotateGlobally(WAngle::MakeFromRadian(0), WAngle::MakeFromRadian(0), WAngle::MakeFromRadian(fRotateHorizontal));
  }

  if (m_Mode == Mode::Pan)
  {
    const float fSpeedFactor = GetCameraSpeed();

    const float fMoveUp = -diffNorm.y * fSpeedFactor;
    const float fMoveRight = diffNorm.x * fSpeedFactor;

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);
  }

  return WEditorInput::WasExclusivelyHandled;
}

WEditorInput WOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_Mode != Mode::Off)
    return WEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (!m_pCamera->IsPerspective())
    return WEditorInput::MayBeHandledByOthers;

  const float fScale = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 1.4f : 1.1f;

  const WVec3 vOrbitPoint = m_Volume.GetCenter();

  float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
  {
    fDistance /= fScale;
  }
  else
  {
    fDistance *= fScale;
  }

  WVec3 vDir = m_pCamera->GetDirForwards();
  if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
  {
    vDir.Set(1.0f, 0, 0);
  }

  m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, WVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return WEditorInput::WasExclusivelyHandled;
}


WEditorInput WOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_F))
  {
    MoveCameraToDefaultPosition();
    return WEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode != Mode::Free)
    return WEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_W))
  {
    m_bMoveForwards = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_S))
  {
    m_bMoveBackwards = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_A))
  {
    m_bMoveLeft = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_D))
  {
    m_bMoveRight = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_Q))
  {
    m_bMoveDown = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_E))
  {
    m_bMoveUp = true;
    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WOrbitCameraContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return WEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return WEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_W))
  {
    m_bMoveForwards = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_S))
  {
    m_bMoveBackwards = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_A))
  {
    m_bMoveLeft = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_D))
  {
    m_bMoveRight = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_Q))
  {
    m_bMoveDown = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  if (WQtUtils::IsEquivalentQtKey(e, Qt::Key_E))
  {
    m_bMoveUp = false;
    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;
}

void WOrbitCameraContext::UpdateContext()
{
  WTime diff = WTime::Now() - m_LastUpdate;
  m_LastUpdate = WTime::Now();

  const double TimeDiff = WMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;

  fSpeedFactor *= GetCameraSpeed();

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(0, 0, 1 * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(0, 0, -1 * fSpeedFactor);
}

float WOrbitCameraContext::GetCameraSpeed() const
{
  const WVec3 ext = m_Volume.GetHalfExtents();
  float fSize = WMath::Max(0.1f, ext.x, ext.y, ext.z);

  return fSize;
}

void WOrbitCameraContext::ResetCursor()
{
  if (m_Mode == Mode::Off)
  {
    SetMouseMode(WEditorInputContext::MouseMode::Normal);
    MakeActiveInputContext(false);
  }
}

void WOrbitCameraContext::SetCurrentMouseMode()
{
  if (m_Mode != Mode::Off)
  {
    SetMouseMode(WEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(WEditorInputContext::MouseMode::Normal);
  }
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <QLabel>
#include <QTimer>

WUInt32 WQtEngineViewWidget::s_uiNextViewID = 0;

WQtEngineViewWidget::InteractionContext WQtEngineViewWidget::s_InteractionContext;

void WObjectPickingResult::Reset()
{
  m_PickedComponent = WUuid();
  m_PickedObject = WUuid();
  m_PickedOther = WUuid();
  m_uiPartIndex = 0;
  m_vPickedPosition.SetZero();
  m_vPickedNormal.SetZero();
  m_vPickingRayStart.SetZero();
}

/// Small helper class which exposes the native surface that the renderer can render into.
class WQtNativeSurfaceWidget : public QWidget
{
public:
  WQtNativeSurfaceWidget(QWidget* pParent = nullptr)
    : QWidget(pParent)
  {
    // setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setMinimumSize(64, 64); // prevent the window from becoming zero sized, otherwise the rendering code may crash

    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground);
  }

  virtual void paintEvent(QPaintEvent* pEvent) override {}
  virtual QPaintEngine* paintEngine() const override { return nullptr; }
};

////////////////////////////////////////////////////////////////////////
// WQtEngineViewWidget public functions
////////////////////////////////////////////////////////////////////////

WSizeU32 WQtEngineViewWidget::s_FixedResolution(0, 0);

WQtEngineViewWidget::WQtEngineViewWidget(QWidget* pParent, WQtEngineDocumentWindow* pDocumentWindow, WEngineViewConfig* pViewConfig)
  : QWidget(pParent)
  , m_pDocumentWindow(pDocumentWindow)
  , m_pViewConfig(pViewConfig)
{
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64);
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);

  m_pMainLayout = new QHBoxLayout(this);
  m_pMainLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pMainLayout);

  // We use a timer instead of resizing the engine viewport when the parent window resizes, because Nvidia driver deadlocks on Linux if you do so in rapid succession.
  m_pResizeTimer = new QTimer(this);
  m_pResizeTimer->setSingleShot(true);
  m_pResizeTimer->setInterval(100);
  connect(m_pResizeTimer, &QTimer::timeout, this, [this]()
    {
      if (m_pViewportWidget && !s_FixedResolution.HasNonZeroArea())
      {
        m_pViewportWidget->setGeometry(0, 0, width(), height());
        m_pDocumentWindow->TriggerRedraw();
      } });

  RecreateEngineViewport();

  m_bUpdatePickingData = false;
  m_bInDragAndDropOperation = false;

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;

  m_fCameraLerp = 1.0f;
  m_fCameraTargetFovOrDim = 70.0f;

  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WQtEngineViewWidget::EngineViewProcessEventHandler, this));

  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    ShowRestartButton(true);
}


WQtEngineViewWidget::~WQtEngineViewWidget()
{
  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WQtEngineViewWidget::EngineViewProcessEventHandler, this));

  {
    // Ensure the engine process swap chain is destroyed before the window.
    WViewDestroyedMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    // If we fail to send the message the engine process is down and we don't need to clean up.
    if (m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg))
    {
      // Wait for engine process response
      auto callback = [&](WProcessMessage* pMsg) -> bool
      {
        auto pResponse = static_cast<WViewDestroyedResponseMsgToEditor*>(pMsg);
        return pResponse->m_DocumentGuid == m_pDocumentWindow->GetDocument()->GetGuid() && pResponse->m_uiViewID == msg.m_uiViewID;
      };
      WProcessCommunicationChannel::WaitForMessageCallback cb = callback;

      if (WEditorEngineProcessConnection::GetSingleton()->WaitForMessage(WGetStaticRTTI<WViewDestroyedResponseMsgToEditor>(), WTime::MakeFromSeconds(5), &cb).Failed())
      {
        WLog::Error("Timeout while waiting for engine process to destroy view.");
      }
    }
  }

  m_pDocumentWindow->RemoveViewWidget(this);
}

void WQtEngineViewWidget::SyncToEngine()
{
  WViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;

  float fov = m_pViewConfig->m_Camera.GetFovOrDim();
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    WEditorPreferencesUser* pPref = WPreferences::QueryPreferences<WEditorPreferencesUser>();
    fov = pPref->m_fPerspectiveFieldOfView;
  }

  cam.m_uiViewID = GetViewID();
  cam.m_fNearPlane = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode = (WInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_bUseCameraTransformOnDevice = m_pViewConfig->m_bUseCameraTransformOnDevice;
  cam.m_fFovOrDim = fov;
  cam.m_vDirForwards = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_pViewConfig->m_Camera.GetCenterPosition();
  cam.m_ViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), cam.m_ProjMatrix);

  cam.m_uiHWND = (WUInt64)(m_pViewportWidget->winId());
  cam.m_uiWindowWidth = m_pViewportWidget->width() * this->devicePixelRatio();
  cam.m_uiWindowHeight = m_pViewportWidget->height() * this->devicePixelRatio();
  cam.m_bUpdatePickingData = m_bUpdatePickingData;
  cam.m_bEnablePickingSelected = IsPickingAgainstSelectionAllowed() && (!WEditorInputContext::IsAnyInputContextActive() || WEditorInputContext::GetActiveInputContext()->IsPickingSelectedAllowed());
  cam.m_bEnablePickTransparent = m_bPickTransparent;

  if (s_FixedResolution.HasNonZeroArea())
  {
    cam.m_uiWindowWidth = s_FixedResolution.width;
    cam.m_uiWindowHeight = s_FixedResolution.height;
  }

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}


void WQtEngineViewWidget::GetCameraMatrices(WMat4& out_mViewMatrix, WMat4& out_mProjectionMatrix) const
{
  out_mViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), out_mProjectionMatrix);
}

void WQtEngineViewWidget::UpdateCameraInterpolation()
{
  if (m_fCameraLerp >= 1.0f)
    return;

  const WTime tNow = WTime::Now();
  const WTime tDiff = tNow - m_LastCameraUpdate;
  m_LastCameraUpdate = tNow;

  m_fCameraLerp += tDiff.GetSeconds() * 3.0f;

  if (m_fCameraLerp >= 1.0f)
    m_fCameraLerp = 1.0f;

  WCamera& cam = m_pViewConfig->m_Camera;

  const float fLerpValue = WMath::Sin(WAngle::MakeFromDegree(90.0f * m_fCameraLerp));

  WQuat qRot, qRotFinal;
  qRot = WQuat::MakeShortestRotation(m_vCameraStartDirection, m_vCameraTargetDirection);
  qRotFinal = WQuat::MakeSlerp(WQuat::MakeIdentity(), qRot, fLerpValue);

  const WVec3 vNewDirection = qRotFinal * m_vCameraStartDirection;
  const WVec3 vNewPosition = WMath::Lerp(m_vCameraStartPosition, m_vCameraTargetPosition, fLerpValue);
  const float fNewFovOrDim = WMath::Lerp(m_fCameraStartFovOrDim, m_fCameraTargetFovOrDim, fLerpValue);

  /// \todo Hard coded up vector
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, m_vCameraUp);
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void WQtEngineViewWidget::InterpolateCameraTo(const WVec3& vPosition, const WVec3& vDirection, float fFovOrDim, const WVec3* pNewUpDirection /*= nullptr*/, bool bImmediate /*= false*/)
{
  m_vCameraStartPosition = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  if (pNewUpDirection)
    m_vCameraUp = *pNewUpDirection;
  else
    m_vCameraUp = m_pViewConfig->m_Camera.GetCenterDirUp();

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();
  m_vCameraUp.Normalize();


  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();

  if (fFovOrDim > 0.0f)
    m_fCameraTargetFovOrDim = fFovOrDim;


  W_ASSERT_DEV(m_fCameraTargetFovOrDim > 0, "Invalid FOV or ortho dimension");

  if (m_vCameraStartPosition == m_vCameraTargetPosition && m_vCameraStartDirection == m_vCameraTargetDirection && m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = WTime::Now();
  m_fCameraLerp = 0.0f;

  if (bImmediate)
  {
    // make sure the next camera update interpolates all the way
    m_LastCameraUpdate -= WTime::MakeFromSeconds(10);
    m_fCameraLerp = 0.9f;
  }
}

void WQtEngineViewWidget::SetEnablePicking(bool bEnable)
{
  m_bUpdatePickingData = bEnable;
}

void WQtEngineViewWidget::SetPickTransparent(bool bEnable)
{
  if (m_bPickTransparent == bEnable)
    return;

  m_bPickTransparent = bEnable;
  m_LastPickingResult.Reset();
}

void WQtEngineViewWidget::OpenContextMenu(QPoint globalPos)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  OnOpenContextMenu(globalPos);
}


const WObjectPickingResult& WQtEngineViewWidget::PickObject(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY) const
{
  if (!WEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    m_LastPickingResult.Reset();
  }
  else
  {
    WViewPickingMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    msg.m_uiPickPosX = uiScreenPosX * devicePixelRatio();
    msg.m_uiPickPosY = uiScreenPosY * devicePixelRatio();

    GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }

  return m_LastPickingResult;
}

void WQtEngineViewWidget::ClearLastPickedObject()
{
  m_LastPickingResult.Reset();
}

WResult WQtEngineViewWidget::PickPlane(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY, const WPlane& plane, WVec3& out_vPosition) const
{
  const auto& cam = m_pViewConfig->m_Camera;

  WMat4 mView = cam.GetViewMatrix();
  WMat4 mProj;
  cam.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), mProj);
  WMat4 mViewProj = mProj * mView;
  WMat4 mInvViewProj = mViewProj.GetInverse();

  WVec3 vScreenPos(uiScreenPosX, uiScreenPosY, 0);
  WVec3 vResPos, vResRay;

  if (WGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_pViewportWidget->width(), m_pViewportWidget->height(), vScreenPos, vResPos, &vResRay).Failed())
    return W_FAILURE;

  if (plane.GetRayIntersection(vResPos, vResRay, nullptr, &out_vPosition))
    return W_SUCCESS;

  return W_FAILURE;
}

void WQtEngineViewWidget::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
  if (const WViewPickingResultMsgToEditor* pFullMsg = WDynamicCast<const WViewPickingResultMsgToEditor*>(pMsg))
  {
    m_LastPickingResult.m_PickedObject = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return;
  }
  else if (const WViewMarqueePickingResultMsgToEditor* pFullMsg = WDynamicCast<const WViewMarqueePickingResultMsgToEditor*>(pMsg))
  {
    HandleMarqueePickingResult(pFullMsg);
    return;
  }
}

WPlane WQtEngineViewWidget::GetFallbackPickingPlane(WVec3 vPointOnPlane) const
{
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    return WPlane::MakeFromNormalAndPoint(WVec3(0, 0, 1), vPointOnPlane);
  }
  else
  {
    return WPlane::MakeFromNormalAndPoint(-m_pViewConfig->m_Camera.GetCenterDirForwards(), vPointOnPlane);
  }
}

void WQtEngineViewWidget::TakeScreenshot(const char* szOutputPath) const
{
  WViewScreenshotMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  msg.m_sOutputFile = szOutputPath;
  m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
}

////////////////////////////////////////////////////////////////////////
// WQtEngineViewWidget qt overrides
////////////////////////////////////////////////////////////////////////

bool WQtEngineViewWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::ShortcutOverride)
  {
    if (WEditorInputContext::IsAnyInputContextActive())
    {
      // if the active input context does not like other shortcuts,
      // accept this event and thus block further shortcut processing
      // instead Qt will then send a keypress event
      if (WEditorInputContext::GetActiveInputContext()->GetShortcutsDisabled())
        event->accept();
    }
  }

  return false;
}


void WQtEngineViewWidget::paintEvent(QPaintEvent* event)
{
  // event->accept();
}

void WQtEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  if (s_FixedResolution.HasNonZeroArea())
  {
    m_pDocumentWindow->TriggerRedraw();
    return;
  }

  // Defer the viewport resize to avoid recreating the swapchain on every intermediate size
  // while the user is still dragging the window border.
  m_pResizeTimer->start();
}

void WQtEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->KeyPressEvent(e) == WEditorInput::WasExclusivelyHandled)
      return;
  }

  if (WEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyPressEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyPressEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyPressEvent(e);
}

void WQtEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->KeyReleaseEvent(e) == WEditorInput::WasExclusivelyHandled)
      return;
  }

  if (WEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyReleaseEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyReleaseEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void WQtEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->MousePressEvent(e) == WEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (WEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MousePressEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MousePressEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mousePressEvent(e);
}

void WQtEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->MouseReleaseEvent(e) == WEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (WEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseReleaseEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseReleaseEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseReleaseEvent(e);
}

void WQtEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  // kick off the picking
  PickObject(e->pos().x(), e->pos().y());

  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->MouseMoveEvent(e) == WEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (WEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseMoveEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseMoveEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseMoveEvent(e);
}

void WQtEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    if (WEditorInputContext::GetActiveInputContext()->WheelEvent(e) == WEditorInput::WasExclusivelyHandled)
      return;
  }

  if (WEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    WEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->WheelEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->WheelEvent(e) == WEditorInput::WasExclusivelyHandled || WEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::wheelEvent(e);
}

void WQtEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (WEditorInputContext::IsAnyInputContextActive())
  {
    WEditorInputContext::GetActiveInputContext()->FocusLost(false);
    WEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void WQtEngineViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  m_bInDragAndDropOperation = true;
}


void WQtEngineViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  m_bInDragAndDropOperation = false;
}


void WQtEngineViewWidget::dropEvent(QDropEvent* e)
{
  m_bInDragAndDropOperation = false;
}


////////////////////////////////////////////////////////////////////////
// WQtEngineViewWidget protected functions
////////////////////////////////////////////////////////////////////////

void WQtEngineViewWidget::EngineViewProcessEventHandler(const WEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case WEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowProcessStuckIndicator(false);
      ShowRestartButton(true);
    }
    break;

    case WEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      RecreateEngineViewport();
      ShowRestartButton(false);
    }
    break;

    case WEditorEngineProcessConnection::Event::Type::ProcessMessage:
      break;

    case WEditorEngineProcessConnection::Event::Type::Invalid:
      W_ASSERT_DEV(false, "Invalid message should never happen");
      break;

    case WEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case WEditorEngineProcessConnection::Event::Type::ProcessRestarted:
    case WEditorEngineProcessConnection::Event::Type::ProcessUnstuck:
      ShowProcessStuckIndicator(false);
      break;

    case WEditorEngineProcessConnection::Event::Type::ProcessStuck:
      ShowProcessStuckIndicator(true);
      break;
  }
}

void WQtEngineViewWidget::ShowRestartButton(bool bShow)
{
  WQtScopedUpdatesDisabled _(this);

  if (m_pRestartButton == nullptr && bShow == true)
  {
    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &WQtEngineViewWidget::SlotRestartEngineProcess);

    m_pMainLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }

  m_pViewportWidget->setVisible(!bShow);
}

void WQtEngineViewWidget::ShowProcessStuckIndicator(bool bShow)
{
  if (m_pStuckIndicator == nullptr && bShow)
  {
    m_pStuckIndicator = new QWidget(m_pViewportWidget);
    m_pStuckIndicator->setAttribute(Qt::WA_TransparentForMouseEvents);

    QHBoxLayout* pLayout = new QHBoxLayout(m_pStuckIndicator);
    pLayout->setContentsMargins(8, 8, 8, 8);

    QLabel* pIcon = new QLabel(m_pStuckIndicator);
    pIcon->setPixmap(QIcon(":/GuiFoundation/Icons/Warning.svg").pixmap(32, 32));
    pIcon->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pLayout->addWidget(pIcon);

    WOsProcessID engineProcess = WEditorEngineProcessConnection::GetSingleton()->GetEngineProcessID();
    WStringBuilder sText;
    sText.SetFormat("Viewport Stalled, ProcessId: {}\nThe engine process might be busy or ran into a problem.", engineProcess);

    QLabel* pText = new QLabel(WMakeQString(sText), m_pStuckIndicator);
    pText->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    pLayout->addWidget(pText);

    m_pStuckIndicator->adjustSize();
    m_pStuckIndicator->setFixedWidth(5000);
  }

  if (m_pStuckIndicator)
  {
    m_pStuckIndicator->setVisible(bShow);

    if (bShow)
    {
      m_pStuckIndicator->move(0, 0);
      m_pStuckIndicator->raise();
    }
  }
}

void WQtEngineViewWidget::RecreateEngineViewport()
{
  if (m_pViewportWidget)
  {
    m_pStuckIndicator = nullptr;
    m_pViewportWidget->removeEventFilter(this);
    m_pViewportWidget->hide();
    m_pViewportWidget->setParent(nullptr);
    m_pViewportWidget->deleteLater();
  }

  m_pViewportWidget = new WQtNativeSurfaceWidget(this);
  m_pViewportWidget->installEventFilter(this);
  m_pViewportWidget->setFocusProxy(this);
  if (s_FixedResolution.HasNonZeroArea())
  {
    qreal pixelRatio = devicePixelRatio();
    // When using DPI scaling, this could actually not be possible to achieve so we use the ceiling of the logical size. This is fine, as the editor tests crop the resulting image if not of the proper size.
    m_pViewportWidget->setFixedSize(
      static_cast<WInt32>(WMath::Ceil(s_FixedResolution.width / pixelRatio)),
      static_cast<WInt32>(WMath::Ceil(s_FixedResolution.height / pixelRatio)));
  }
  else
  {
    // Don't add the viewport widget to the layout. Instead, it is resized manually via
    // a timer in resizeEvent to debounce resize events during interactive window resizing.
    m_pViewportWidget->setGeometry(0, 0, width(), height());
  }
}

////////////////////////////////////////////////////////////////////////
// WQtEngineViewWidget private slots
////////////////////////////////////////////////////////////////////////

void WQtEngineViewWidget::SlotRestartEngineProcess()
{
  WEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}


////////////////////////////////////////////////////////////////////////
// WQtViewWidgetContainer
////////////////////////////////////////////////////////////////////////

WQtViewWidgetContainer::WQtViewWidgetContainer(ads::CDockManager* pDockManager, QWidget* pParent, WQtEngineViewWidget* pViewWidget, const char* szToolBarMapping)
  : ads::CDockWidget(pDockManager, "3D View", pParent)
{
  setObjectName("WQtViewWidgetContainer");

  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetMovable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFocusable, true);

  // need contrast with the rest of the widgets around it
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  QWidget* pDummy = new QWidget();
  pDummy->setObjectName("Dummy");

  m_pLayout = new QVBoxLayout(pDummy);
  m_pLayout->setObjectName("QVBoxLayout1");
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  pDummy->setLayout(m_pLayout);

  m_pViewWidget = pViewWidget;
  m_pViewWidget->setParent(pDummy);

  if (!WStringUtils::IsNullOrEmpty(szToolBarMapping))
  {
    // Add Tool Bar
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = szToolBarMapping;
    context.m_pDocument = pViewWidget->GetDocumentWindow()->GetDocument();
    context.m_pWindow = m_pViewWidget;
    pToolBar->SetActionContext(context);
    m_pLayout->addWidget(pToolBar, 0);
  }

  m_pLayout->addWidget(m_pViewWidget, 1);

  setWidget(pDummy);
}

WQtViewWidgetContainer::~WQtViewWidgetContainer() = default;

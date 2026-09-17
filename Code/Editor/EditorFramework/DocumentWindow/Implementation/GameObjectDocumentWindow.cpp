#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

WQtGameObjectDocumentWindow::WQtGameObjectDocumentWindow(WGameObjectDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  pDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  WSnapProvider::s_Events.AddEventHandler(WMakeDelegate(&WQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

WQtGameObjectDocumentWindow::~WQtGameObjectDocumentWindow()
{
  GetGameObjectDocument()->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  WSnapProvider::s_Events.RemoveEventHandler(WMakeDelegate(&WQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

WGameObjectDocument* WQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<WGameObjectDocument*>(GetDocument());
}

WWorldSettingsMsgToEngine WQtGameObjectDocumentWindow::GetWorldSettings() const
{
  WWorldSettingsMsgToEngine msg;
  auto pGameObjectDoc = GetGameObjectDocument();
  msg.m_bRenderOverlay = pGameObjectDoc->GetRenderSelectionOverlay();
  msg.m_bRenderShapeIcons = pGameObjectDoc->GetRenderShapeIcons();
  msg.m_bRenderSelectionBoxes = pGameObjectDoc->GetRenderVisualizers();
  msg.m_bAddAmbientLight = pGameObjectDoc->GetAddAmbientLight();
  return msg;
}

WGridSettingsMsgToEngine WQtGameObjectDocumentWindow::GetGridSettings() const
{
  WGridSettingsMsgToEngine msg;

  if (auto pTool = GetGameObjectDocument()->GetActiveEditTool())
  {
    pTool->GetGridSettings(msg);
  }
  else
  {
    WManipulatorAdapterRegistry::GetSingleton()->QueryGridSettings(GetDocument(), msg);
  }

  return msg;
}

void WQtGameObjectDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg)
{
  WQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WQuerySelectionBBoxResultMsgToEditor>())
  {
    const WQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const WQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (msg->m_uiViewID == 0xFFFFFFFF)
    {
      for (auto pView : m_ViewWidgets)
      {
        if (!pView)
          continue;

        if (msg->m_iPurpose == 0)
          HandleFocusOnSelection(msg, static_cast<WQtGameObjectViewWidget*>(pView));
      }
    }
    else
    {
      WQtGameObjectViewWidget* pSceneView = static_cast<WQtGameObjectViewWidget*>(GetViewWidgetByID(msg->m_uiViewID));

      if (!pSceneView)
        return;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return;
  }
}

void WQtGameObjectDocumentWindow::GameObjectEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      FocusOnSelectionHoveredView();
      break;

    case WGameObjectEvent::Type::TriggerFocusOnSelection_All:
      FocusOnSelectionAllViews();
      break;

    default:
      break;
  }
}

void WQtGameObjectDocumentWindow::FocusOnSelectionAllViews()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    return;

  WQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void WQtGameObjectDocumentWindow::FocusOnSelectionHoveredView()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    return;

  auto pView = GetHoveredViewWidget();

  if (pView == nullptr)
    return;

  WQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = pView->GetViewID();
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void WQtGameObjectDocumentWindow::HandleFocusOnSelection(const WQuerySelectionBBoxResultMsgToEditor* pMsg, WQtGameObjectViewWidget* pSceneView)
{
  const WVec3 vPivotPoint = pMsg->m_vCenter;

  const WCamera& cam = pSceneView->m_pViewConfig->m_Camera;

  WVec3 vNewCameraPosition = cam.GetCenterPosition();
  WVec3 vNewCameraDirection = cam.GetDirForwards();
  float fNewFovOrDim = cam.GetFovOrDim();

  if (pSceneView->width() == 0 || pSceneView->height() == 0)
    return;

  const float fApsectRation = (float)pSceneView->width() / (float)pSceneView->height();

  WBoundingBox bbox;

  // clamp the bbox of the selection to ranges that won't break down due to float precision
  {
    bbox = WBoundingBox::MakeFromCenterAndHalfExtents(pMsg->m_vCenter, pMsg->m_vHalfExtents);
    bbox.m_vMin = bbox.m_vMin.CompMax(WVec3(-1000.0f));
    bbox.m_vMax = bbox.m_vMax.CompMin(WVec3(+1000.0f));
  }

  const WVec3 vCurrentOrbitPoint = pSceneView->m_pCameraMoveContext->GetOrbitPoint();
  const bool bZoomIn = vPivotPoint.IsEqual(vCurrentOrbitPoint, 0.1f);

  if (cam.GetCameraMode() == WCameraMode::PerspectiveFixedFovX || cam.GetCameraMode() == WCameraMode::PerspectiveFixedFovY)
  {
    const float maxExt = pMsg->m_vHalfExtents.GetLength();
    const float fMinDistance = cam.GetNearPlane() * 1.1f + maxExt;

    {
      WPlane p;
      p = WPlane::MakeFromNormalAndPoint(vNewCameraDirection, vNewCameraPosition);

      // at some distance the floating point precision gets so crappy that the camera movement breaks
      // therefore we clamp it to a 'reasonable' distance here
      const float distBest = WMath::Min(WMath::Abs(p.GetDistanceTo(vPivotPoint)), 500.0f);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * WMath::Max(fMinDistance, distBest);
    }

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (!pMsg->m_vHalfExtents.IsZero(WMath::DefaultEpsilon<float>()) && bZoomIn)
    {
      const WAngle fovX = cam.GetFovX(fApsectRation);
      const WAngle fovY = cam.GetFovY(fApsectRation);

      const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

      const float dist1 = fRadius / WMath::Sin(fovX * 0.75f);
      const float dist2 = fRadius / WMath::Sin(fovY * 0.75f);
      const float distBest = WMath::Max(dist1, dist2);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * WMath::Max(fMinDistance, distBest);
    }
  }
  else
  {
    vNewCameraPosition = pMsg->m_vCenter;

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (bZoomIn)
    {

      const WVec3 right = cam.GetDirRight();
      const WVec3 up = cam.GetDirUp();

      const float fSizeFactor = 2.0f;

      const float fRequiredWidth = WMath::Abs(right.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;
      const float fRequiredHeight = WMath::Abs(up.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;

      float fDimWidth, fDimHeight;

      if (cam.GetCameraMode() == WCameraMode::OrthoFixedHeight)
      {
        fDimHeight = cam.GetFovOrDim();
        fDimWidth = fDimHeight * fApsectRation;
      }
      else
      {
        fDimWidth = cam.GetFovOrDim();
        fDimHeight = fDimWidth / fApsectRation;
      }

      const float fScaleWidth = fRequiredWidth / fDimWidth;
      const float fScaleHeight = fRequiredHeight / fDimHeight;

      const float fScaleDim = WMath::Max(fScaleWidth, fScaleHeight);

      if (fScaleDim > 0.0f)
      {
        fNewFovOrDim *= fScaleDim;
      }
    }
  }

  pSceneView->m_pCameraMoveContext->SetOrbitDistance(vPivotPoint.GetDistanceTo(vNewCameraPosition));
  pSceneView->InterpolateCameraTo(vNewCameraPosition, vNewCameraDirection, fNewFovOrDim);
}

void WQtGameObjectDocumentWindow::SnapProviderEventHandler(const WSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
    case WSnapProviderEvent::Type::RotationSnapChanged:
      ShowTemporaryStatusBarMsg(WFmt(WStringUtf8(L"Snapping Angle: {0}°").GetData(), WSnapProvider::GetRotationSnapValue().GetDegree()));
      break;

    case WSnapProviderEvent::Type::ScaleSnapChanged:
      ShowTemporaryStatusBarMsg(WFmt("Snapping Value: {0}", WSnapProvider::GetScaleSnapValue()));
      break;

    case WSnapProviderEvent::Type::TranslationSnapChanged:
      ShowTemporaryStatusBarMsg(WFmt("Snapping Value: {0}", WSnapProvider::GetTranslationSnapValue()));
      break;
  }
}

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

WQtSkeletonAssetDocumentWindow::WQtSkeletonAssetDocumentWindow(WSkeletonAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "SkeletonAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "SkeletonAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SkeletonAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  WQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(-1.6f, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig, true);
    m_pViewWidget->ConfigureRelative(WVec3(0, 0, 1), WVec3(5.0f), WVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "SkeletonAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Tree View
  {
    WQtDocumentPanel* pPanelTree = new WQtSkeletonPanel(GetContainerWindow()->GetDockManager(), this, static_cast<WSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }

  pDocument->Events().AddEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::CommandEventHandler, this));

  FinishWindowCreation();
}

WQtSkeletonAssetDocumentWindow::~WQtSkeletonAssetDocumentWindow()
{
  static_cast<WSkeletonAssetDocument*>(GetDocument())->Events().RemoveEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::CommandEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

WSkeletonAssetDocument* WQtSkeletonAssetDocumentWindow::GetSkeletonDocument()
{
  return static_cast<WSkeletonAssetDocument*>(GetDocument());
}

void WQtSkeletonAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto* pDoc = GetSkeletonDocument();

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderBones";
    msg.m_PayloadValue = pDoc->GetRenderBones();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderColliders";
    msg.m_PayloadValue = pDoc->GetRenderColliders();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderJoints";
    msg.m_PayloadValue = pDoc->GetRenderJoints();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderSwingLimits";
    msg.m_PayloadValue = pDoc->GetRenderSwingLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderTwistLimits";
    msg.m_PayloadValue = pDoc->GetRenderTwistLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";

    if (pDoc->GetRenderPreviewMesh())
      msg.m_sPayload = pDoc->GetProperties()->m_sPreviewMesh;
    else
      msg.m_sPayload = "";

    GetDocument()->SendMessageToEngine(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void WQtSkeletonAssetDocumentWindow::QueryObjectBBox(WInt32 iPurpose /*= 0*/)
{
  WQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}


void WQtSkeletonAssetDocumentWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  WStringBuilder filter;

  switch (e.m_Type)
  {
    case WSelectionManagerEvent::Type::SelectionCleared:
    case WSelectionManagerEvent::Type::SelectionSet:
    case WSelectionManagerEvent::Type::ObjectAdded:
    case WSelectionManagerEvent::Type::ObjectRemoved:
    {
      const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

      for (auto pObj : sel)
      {
        WVariant name = pObj->GetTypeAccessor().GetValue("Name");
        if (name.IsValid() && name.CanConvertTo<WString>())
        {
          filter.Append(name.ConvertTo<WString>().GetData(), ";");
        }
      }

      WSimpleDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "HighlightBones";
      msg.m_sPayload = filter;

      GetDocument()->SendMessageToEngine(&msg);
    }
    break;

    case WSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void WQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler(const WSkeletonAssetEvent& e)
{
  if (e.m_Type == WSkeletonAssetEvent::Transformed)
  {
    SendLiveResourcePreview();
  }
}

void WQtSkeletonAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  // additionally do live updates for these specific properties
  if (e.m_sProperty == "LocalRotation" ||                                                 // joint offset rotation
      e.m_sProperty == "Offset" || e.m_sProperty == "Rotation" ||                         // all shapes
      e.m_sProperty == "Radius" || e.m_sProperty == "Length" ||                           // sphere and capsule
      e.m_sProperty == "Width" || e.m_sProperty == "Thickness" ||                         // box
      e.m_sProperty == "SwingLimitY" || e.m_sProperty == "SwingLimitZ" ||                 // joint swing limit
      e.m_sProperty == "TwistLimitHalfAngle" || e.m_sProperty == "TwistLimitCenterAngle") // joint twist limit
  {
    SendLiveResourcePreview();
  }
}

void WQtSkeletonAssetDocumentWindow::CommandEventHandler(const WCommandHistoryEvent& e)
{
  if (e.m_Type == WCommandHistoryEvent::Type::TransactionEnded || e.m_Type == WCommandHistoryEvent::Type::UndoEnded || e.m_Type == WCommandHistoryEvent::Type::RedoEnded)
  {
    SendLiveResourcePreview();
  }
}

void WQtSkeletonAssetDocumentWindow::SendLiveResourcePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WSkeletonAssetDocument* pDoc = WDynamicCast<WSkeletonAssetDocument*>(GetDocument());

  if (pDoc->m_bIsTransforming)
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);


  // Write Path
  WStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WSkeleton");

  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(pDoc->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter, *pDoc->GetProperties()).AssertSuccess();
  msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtSkeletonAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtSkeletonAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtSkeletonAssetDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WQuerySelectionBBoxResultMsgToEditor>())
  {
    const WQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const WQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(WVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  WQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}

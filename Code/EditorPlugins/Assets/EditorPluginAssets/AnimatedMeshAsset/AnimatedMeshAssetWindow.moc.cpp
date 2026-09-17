#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>

WQtAnimatedMeshAssetDocumentWindow::WQtAnimatedMeshAssetDocumentWindow(WAnimatedMeshAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "AnimatedMeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "AnimatedMeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimatedMeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  WQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(-1.6f, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(WVec3(0, 0, 1), WVec3(10.0f), WVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);

    m_pCameraFlyContext = W_DEFAULT_NEW(WCameraMoveContext, this, m_pViewWidget);
    m_pCameraFlyContext->SetCamera(&m_ViewConfig.m_Camera);
    m_pCameraFlyContext->LoadState();

    pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "AnimatedMeshAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AnimatedMeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");
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

  FinishWindowCreation();

  UpdatePreview();

  m_pHighlightTimer = new QTimer();
  connect(m_pHighlightTimer, &QTimer::timeout, this, &WQtAnimatedMeshAssetDocumentWindow::HighlightTimer);
  m_pHighlightTimer->setInterval(500);
  m_pHighlightTimer->start();
}

WQtAnimatedMeshAssetDocumentWindow::~WQtAnimatedMeshAssetDocumentWindow()
{
  m_pHighlightTimer->stop();

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));
}

WAnimatedMeshAssetDocument* WQtAnimatedMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<WAnimatedMeshAssetDocument*>(GetDocument());
}

void WQtAnimatedMeshAssetDocumentWindow::SetCameraMode(int iMode)
{
  if (m_iCameraMode == iMode)
    return;
  m_iCameraMode = iMode;

  m_pViewWidget->m_InputContexts.Clear();
  if (iMode == 0) // Orbit
    m_pViewWidget->m_InputContexts.PushBack(m_pViewWidget->GetOrbitCamera());
  else            // Free fly
    m_pViewWidget->m_InputContexts.PushBack(m_pCameraFlyContext.Borrow());
}

void WQtAnimatedMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void WQtAnimatedMeshAssetDocumentWindow::QueryObjectBBox(WInt32 iPurpose /* = 0*/)
{
  WQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void WQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  // if (e.m_sProperty == "Resource") // any material change
  {
    UpdatePreview();
  }
}

bool WQtAnimatedMeshAssetDocumentWindow::UpdatePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return false;

  if (GetMeshDocument()->GetProperties() == nullptr)
    return false;

  const auto& materials = GetMeshDocument()->GetProperties()->m_Slots;

  WEditorEngineSetMaterialsMsg msg;
  msg.m_Materials.SetCount(materials.GetCount());

  WUInt32 uiSlot = 0;
  bool bHighlighted = false;

  for (WUInt32 i = 0; i < materials.GetCount(); ++i)
  {
    msg.m_Materials[i] = materials[i].m_sResource;

    if (materials[i].m_bHighlight)
    {
      if (uiSlot == m_uiHighlightSlots)
      {
        bHighlighted = true;
        msg.m_Materials[i] = "Editor/Materials/HighlightMesh.WMaterial";
      }

      ++uiSlot;
    }
  }

  GetEditorEngineConnection()->SendMessage(&msg);

  return bHighlighted;
}

void WQtAnimatedMeshAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtAnimatedMeshAssetDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg)
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

void WQtAnimatedMeshAssetDocumentWindow::HighlightTimer()
{
  if (m_uiHighlightSlots & W_BIT(31))
    m_uiHighlightSlots &= ~W_BIT(31);
  else
    m_uiHighlightSlots |= W_BIT(31);

  if (m_uiHighlightSlots & W_BIT(31))
  {
    UpdatePreview();
  }
  else
  {
    if (UpdatePreview())
    {
      ++m_uiHighlightSlots;
    }
    else
    {
      m_uiHighlightSlots = W_BIT(31);
    }
  }
}

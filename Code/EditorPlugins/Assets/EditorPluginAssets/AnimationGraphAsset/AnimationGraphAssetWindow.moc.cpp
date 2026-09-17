#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetScene.moc.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>



WQtAnimationGraphAssetDocumentWindow::WQtAnimationGraphAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "AnimationGraphAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "AnimationGraphAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationGraphAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new WQtAnimationGraphAssetScene(this);
    m_pScene->InitScene(static_cast<const WVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pView = new WQtVisualGraphView(this);
    m_pView->SetScene(m_pScene);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("WQtDocumentPanel");
    pCentral->setWindowTitle("Anim Graph");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AnimationGraphAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);


    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
  }

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtAnimationGraphAssetDocumentWindow::SelectionEventHandler, this));

  FinishWindowCreation();

  SelectionEventHandler(WSelectionManagerEvent());
}

WQtAnimationGraphAssetDocumentWindow::~WQtAnimationGraphAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAnimationGraphAssetDocumentWindow::SelectionEventHandler, this));
  }
}

void WQtAnimationGraphAssetDocumentWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]()
      {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(((WAnimationGraphAssetDocument*)GetDocument())->GetPropertyObject());
      } });
  }
}

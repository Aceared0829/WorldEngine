#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>



WQtRenderPipelineAssetDocumentWindow::WQtRenderPipelineAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "RenderPipelineAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "RenderPipelineAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("RenderPipelineAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new WQtVisualGraphScene(this);
    m_pScene->InitScene(static_cast<const WVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pView = new WQtVisualGraphView(this);
    m_pView->SetScene(m_pScene);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("PipelineView");
    pCentral->setWindowTitle("Pipeline");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("RenderPipelineAssetDockWidget");
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

  FinishWindowCreation();
}

WQtRenderPipelineAssetDocumentWindow::~WQtRenderPipelineAssetDocumentWindow() = default;

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>


WQtVisualScriptWindow::WQtVisualScriptWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "VisualScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "VisualScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    m_pScene = new WQtVisualScriptNodeScene(this);
    m_pScene->InitScene(static_cast<const WVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pView = new WQtVisualGraphView(this);
    m_pView->SetScene(m_pScene);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("VisualScriptView");
    pCentral->setWindowTitle("Script");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
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

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtVisualScriptWindow::SelectionEventHandler, this));

  FinishWindowCreation();

  SelectionEventHandler(WSelectionManagerEvent());
}

WQtVisualScriptWindow::~WQtVisualScriptWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
      WMakeDelegate(&WQtVisualScriptWindow::SelectionEventHandler, this));
  }
}

void WQtVisualScriptWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1,
      [this]()
      {
        auto pDocument = GetDocument();
        auto pSelectionManager = pDocument->GetSelectionManager();

        // Check again if the selection is empty. This could have changed due to the delayed execution.
        if (pSelectionManager->IsSelectionEmpty())
        {
          pSelectionManager->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
        }
      });
  }
}

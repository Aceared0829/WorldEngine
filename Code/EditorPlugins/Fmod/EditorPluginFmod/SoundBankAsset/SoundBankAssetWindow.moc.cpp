#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

WSoundBankAssetDocumentWindow::WSoundBankAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "SoundBankAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "SoundBankAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundBankAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SoundBankAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Bank Properties");
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

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<WSoundBankAssetDocument*>(pDocument);

  FinishWindowCreation();
}

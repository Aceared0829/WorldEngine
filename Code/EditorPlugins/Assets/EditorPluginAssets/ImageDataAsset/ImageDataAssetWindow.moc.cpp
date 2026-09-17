#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// WQtImageDataAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

WQtImageDataAssetDocumentWindow::WQtImageDataAssetDocumentWindow(WImageDataAssetDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "ImageDataAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "ImageDataAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ImageDataAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pImageWidget = new WQtImageWidget(this);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("ImageDataView");
    pCentral->setWindowTitle("Image");
    pCentral->setWidget(m_pImageWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ImageDataProperties");
    pPropertyPanel->setWindowTitle("Image Properties");

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

  FinishWindowCreation();

  UpdatePreview();

  pDocument->Events().AddEventHandler(WMakeDelegate(&WQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler, this), m_EventUnsubscriper);
}

void WQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler(const WImageDataAssetEvent& e)
{
  if (e.m_Type != WImageDataAssetEvent::Type::Transformed)
    return;

  UpdatePreview();
}

void WQtImageDataAssetDocumentWindow::UpdatePreview()
{
  auto pImageDoc = static_cast<WImageDataAssetDocument*>(GetDocument());

  WStringBuilder path = pImageDoc->GetProperties()->m_sInputFile;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(path))
    return;

  QPixmap pixmap;
  if (!pixmap.load(path.GetData(), nullptr, Qt::AutoColor))
    return;

  m_pImageWidget->SetImage(pixmap);
}

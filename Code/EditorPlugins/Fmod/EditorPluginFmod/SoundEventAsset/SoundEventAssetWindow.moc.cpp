#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

WSoundEventAssetDocumentWindow::WSoundEventAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WSoundEventAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "SoundEventAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "SoundEventAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundEventAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SoundEventAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Event Properties");
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

  m_pAssetDoc = static_cast<WSoundEventAssetDocument*>(pDocument);

  FinishWindowCreation();

  UpdatePreview();
}

WSoundEventAssetDocumentWindow::~WSoundEventAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
    WMakeDelegate(&WSoundEventAssetDocumentWindow::PropertyEventHandler, this));
}

void WSoundEventAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((WSoundEventAssetDocument*)GetDocument())->GetProperties();

  // WStringBuilder s;
  // s.SetFormat("Vertices: {0}\nTriangles: {1}\nSubMeshes: {2}", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  // for (WUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot {0}: {1}", m, prop->m_SlotNames[m]);

  // m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void WSoundEventAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  // if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

WQtBlackboardTemplateAssetDocumentWindow::WQtBlackboardTemplateAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "BlackboardTemplateAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "BlackboardTemplateAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("BlackboardTemplateAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("BlackboardTemplateAssetDockWidget");
    pPropertyPanel->setWindowTitle("BlackboardTemplate Properties");
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


    m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

WQtBlackboardTemplateAssetDocumentWindow::~WQtBlackboardTemplateAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void WQtBlackboardTemplateAssetDocumentWindow::UpdatePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto pDocument = static_cast<WBlackboardTemplateAssetDocument*>(GetDocument());

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "BlackboardTemplate";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  WStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WBlackboardTemplate");
  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(GetDocument()->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDocument->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (pDocument->WriteAsset(memoryWriter, WAssetCurator::GetSingleton()->GetActiveAssetProfile()).Succeeded())
  {
    msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void WQtBlackboardTemplateAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "BlackboardTemplate";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void WQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectRemoved)
  {
    UpdatePreview();
  }
}

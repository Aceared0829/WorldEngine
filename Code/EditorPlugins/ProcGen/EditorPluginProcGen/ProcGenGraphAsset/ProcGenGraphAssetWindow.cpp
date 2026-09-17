#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

WProcGenGraphAssetDocumentWindow::WProcGenGraphAssetDocumentWindow(WProcGenGraphAssetDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::TransactionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::SelectionEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "ProcGenAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "ProcGenAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ProcGenAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new WQtProcGenScene(this);
    m_pScene->InitScene(static_cast<const WVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pView = new WQtVisualGraphView(this);
    m_pView->SetScene(m_pScene);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("ProcGenGraphView");
    pCentral->setWindowTitle("Graph");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ProcGenAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
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

  UpdatePreview();

  FinishWindowCreation();

  SelectionEventHandler(WSelectionManagerEvent());
}

WProcGenGraphAssetDocumentWindow::~WProcGenGraphAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::TransactionEventHandler, this));
    GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentWindow::SelectionEventHandler, this));
  }

  RestoreResource();
}

WProcGenGraphAssetDocument* WProcGenGraphAssetDocumentWindow::GetProcGenGraphDocument()
{
  return static_cast<WProcGenGraphAssetDocument*>(GetDocument());
}

void WProcGenGraphAssetDocumentWindow::UpdatePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  WStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WProcGenGraph");
  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(GetDocument()->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetProcGenGraphDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (GetProcGenGraphDocument()->WriteAsset(memoryWriter, WAssetCurator::GetSingleton()->GetActiveAssetProfile(), true).Succeeded())
  {
    msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void WProcGenGraphAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WProcGenGraphAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  // this event is only needed for changes to the DebugPin
  if (e.m_sProperty == "DebugPin")
  {
    UpdatePreview();
  }
  else if (e.m_pObject->GetType() == WGetStaticRTTI<WProcGenGraphAssetProperties>())
  {
    GetProcGenGraphDocument()->UpdateDebugNode();
    UpdatePreview();
  }
}

void WProcGenGraphAssetDocumentWindow::TransactionEventHandler(const WCommandHistoryEvent& e)
{
  if (e.m_Type == WCommandHistoryEvent::Type::TransactionEnded || e.m_Type == WCommandHistoryEvent::Type::UndoEnded || e.m_Type == WCommandHistoryEvent::Type::RedoEnded)
  {
    UpdatePreview();
  }
}

void WProcGenGraphAssetDocumentWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
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
          W_ASSERT_DEV(pDocument, "");
          W_ASSERT_DEV(pDocument->GetObjectManager(), "");
          W_ASSERT_DEV(pDocument->GetObjectManager()->GetRootObject(), "");
          if (pDocument->GetObjectManager()->GetRootObject()->GetChildren().IsEmpty() == false)
          {
            pSelectionManager->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
          }
        }
      });
  }
}

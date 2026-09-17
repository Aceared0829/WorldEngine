#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

WQtPropertyAnimAssetDocumentWindow::WQtPropertyAnimAssetDocumentWindow(WPropertyAnimAssetDocument* pDocument)
  : WQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](WQtEngineDocumentWindow* pWindow, WEngineViewConfig* pConfig) -> WQtEngineViewWidget*
  {
    WQtGameObjectViewWidget* pWidget = new WQtGameObjectViewWidget(nullptr, static_cast<WQtPropertyAnimAssetDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new WQtQuadViewWidget(pDocument, this, ViewFactory, "PropertyAnimAssetViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](WGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<WGameObjectDocument*>(GetDocument()), this, this); });

  pDocument->m_PropertyAnimEvents.AddEventHandler(WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));

  {
    WQtDocumentPanel* pViewPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pViewPanel->setObjectName("WQtDocumentPanel");
    pViewPanel->setWindowTitle("3D View");
    pViewPanel->setWidget(m_pQuadViewWidget);

    m_pDockManager->setCentralWidget(pViewPanel);
  }

  SetTargetFramerate(25);

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "PropertyAnimAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "PropertyAnimAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("PropertyAnimAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Game Object Graph
  {
    std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtGameObjectModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "TempObjects"));
    pModel->AddAdapter(new WQtGameObjectAdapter(pDocument->GetObjectManager()));

    WQtDocumentPanel* pGameObjectPanel = new WQtGameObjectPanel(GetContainerWindow()->GetDockManager(), this, pDocument, "PropertyAnimAsset_ScenegraphContextMenu", std::move(pModel));
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pGameObjectPanel);
  }

  // Property Grid
  {
    WQtDocumentPanel* pPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPanel->setObjectName("PropertyAnimAssetDockWidget");
    pPanel->setWindowTitle("Object Properties");
    pPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
  }

  // Property Tree View
  {
    WQtDocumentPanel* pPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPanel->setObjectName("PropertyAnimPropertiesDockWidget");
    pPanel->setWindowTitle("Animated Properties");
    pPanel->show();

    m_pPropertyTreeView = new WQtPropertyAnimAssetTreeView(pPanel);
    m_pPropertyTreeView->setHeaderHidden(true);
    m_pPropertyTreeView->setRootIsDecorated(true);
    m_pPropertyTreeView->setUniformRowHeights(true);
    m_pPropertyTreeView->setExpandsOnDoubleClick(false);
    pPanel->setWidget(m_pPropertyTreeView);

    connect(m_pPropertyTreeView, &WQtPropertyAnimAssetTreeView::DeleteSelectedItemsEvent, this,
      &WQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems);
    connect(m_pPropertyTreeView, &WQtPropertyAnimAssetTreeView::RebindSelectedItemsEvent, this,
      &WQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems);

    connect(m_pPropertyTreeView, &QTreeView::doubleClicked, this, &WQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked);
    connect(m_pPropertyTreeView, &WQtPropertyAnimAssetTreeView::FrameSelectedItemsEvent, this,
      &WQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks);

    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new WQtPropertyAnimModel(GetPropertyAnimDocument(), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
    m_pPropertyTreeView->expandToDepth(2);
    m_pPropertyTreeView->initialize();
  }

  // Selection Model
  {
    m_pPropertyTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_pPropertyTreeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    m_pSelectionModel = new QItemSelectionModel(m_pPropertiesModel, this);
    m_pPropertyTreeView->setSelectionModel(m_pSelectionModel);

    connect(m_pSelectionModel, &QItemSelectionModel::selectionChanged, this, &WQtPropertyAnimAssetDocumentWindow::onSelectionChanged);
  }

  // Float Curve Panel
  {
    m_pCurvePanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pCurvePanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    m_pCurvePanel->setWindowTitle("Curves");
    m_pCurvePanel->show();

    m_pCurveEditor = new WQtCurve1DEditorWidget(m_pCurvePanel);
    m_pCurvePanel->setWidget(m_pCurveEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pCurvePanel);
  }

  // Color Gradient Panel
  {
    m_pColorGradientPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pColorGradientPanel->setObjectName("PropertyAnimColorGradientDockWidget");
    m_pColorGradientPanel->setWindowTitle("Color Gradient");
    m_pColorGradientPanel->show();

    m_pGradientEditor = new WQtColorGradientEditorWidget(m_pColorGradientPanel);
    m_pColorGradientPanel->setWidget(m_pGradientEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pColorGradientPanel);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pEventTrackPanel->setObjectName("PropertyAnimEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new WQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pEventTrackPanel);
  }

  // Time Scrubber
  {
    m_pScrubberToolbar = new WQtTimeScrubberToolbar(this);
    connect(m_pScrubberToolbar, &WQtTimeScrubberToolbar::ScrubberPosChangedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged);
    connect(m_pScrubberToolbar, &WQtTimeScrubberToolbar::PlayPauseEvent, this, &WQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked);
    connect(m_pScrubberToolbar, &WQtTimeScrubberToolbar::RepeatEvent, this, &WQtPropertyAnimAssetDocumentWindow::onRepeatClicked);
    connect(m_pScrubberToolbar, &WQtTimeScrubberToolbar::DurationChangedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent);
    connect(m_pScrubberToolbar, &WQtTimeScrubberToolbar::AdjustDurationEvent, this, &WQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked);

    addToolBar(Qt::ToolBarArea::BottomToolBarArea, m_pScrubberToolbar);
  }

  // this would show the document properties
  // pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  // Curve editor events
  {
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::InsertCpEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpMovedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpDeletedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentMovedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentLinkEvent, this, &WQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpTangentModeEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginOperationEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndOperationEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginCpChangesEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndCpChangesEvent, this, &WQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges);
  }

  // Gradient editor events
  {
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpAdded, this, &WQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpMoved, this, &WQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpDeleted, this, &WQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpChanged, this, &WQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged);

    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpAdded, this, &WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpMoved, this, &WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpDeleted, this, &WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpChanged, this, &WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged);

    connect(
      m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpAdded, this, &WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded);
    connect(
      m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpMoved, this, &WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpDeleted, this,
      &WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpChanged, this,
      &WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged);

    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::BeginOperation, this, &WQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation);
    connect(m_pGradientEditor, &WQtColorGradientEditorWidget::EndOperation, this, &WQtPropertyAnimAssetDocumentWindow::onGradientEndOperation);

    // connect(m_pGradientEditor, &WQtColorGradientEditorWidget::NormalizeRange, this,
    // &WQtPropertyAnimAssetDocumentWindow::onGradientNormalizeRange);
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::InsertCpEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::CpMovedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::CpDeletedEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted);

    connect(
      m_pEventTrackEditor, &WQtEventTrackEditorWidget::BeginOperationEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation);
    connect(
      m_pEventTrackEditor, &WQtEventTrackEditorWidget::EndOperationEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation);
    connect(
      m_pEventTrackEditor, &WQtEventTrackEditorWidget::BeginCpChangesEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(
      m_pEventTrackEditor, &WQtEventTrackEditorWidget::EndCpChangesEvent, this, &WQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  // GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));

  FinishWindowCreation();

  {
    const WUInt64 uiDuration = GetPropertyAnimDocument()->GetAnimationDurationTicks();
    m_pScrubberToolbar->SetDuration(uiDuration);
  }

  UpdateCurveEditor();
  UpdateGradientEditor();
  UpdateEventTrackEditor();
}

WQtPropertyAnimAssetDocumentWindow::~WQtPropertyAnimAssetDocumentWindow()
{
  GetPropertyAnimDocument()->m_PropertyAnimEvents.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));
  // GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));
}

void WQtPropertyAnimAssetDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}

WObjectAccessorBase* WQtPropertyAnimAssetDocumentWindow::GetObjectAccessor()
{
  return GetPropertyAnimDocument()->GetObjectAccessor();
}

bool WQtPropertyAnimAssetDocumentWindow::CanDuplicateSelection() const
{
  return false;
}

void WQtPropertyAnimAssetDocumentWindow::DuplicateSelection()
{
  W_ASSERT_NOT_IMPLEMENTED;
}


void WQtPropertyAnimAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  {
    // do not try to redraw while the process is crashed, it is obviously futile
    if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
      return;

    {
      WSimulationSettingsMsgToEngine msg;
      msg.m_bSimulateWorld = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      WGridSettingsMsgToEngine msg = GetGridSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      WWorldSettingsMsgToEngine msg = GetWorldSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    GetGameObjectDocument()->SendObjectSelection();

    auto pHoveredView = GetHoveredViewWidget();

    for (auto pView : m_ViewWidgets)
    {
      pView->SetEnablePicking(pView == pHoveredView);
      pView->UpdateCameraInterpolation();
      pView->SyncToEngine();
    }
  }
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler(const WPropertyAnimAssetDocumentEvent& e)
{
  if (e.m_Type == WPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged)
  {
    const WUInt64 uiDuration = e.m_pDocument->GetAnimationDurationTicks();

    m_pScrubberToolbar->SetDuration(uiDuration);
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
  else if (e.m_Type == WPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged)
  {
    m_pScrubberToolbar->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pCurveEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pGradientEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pEventTrackEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
  }
  else if (e.m_Type == WPropertyAnimAssetDocumentEvent::Type::PlaybackChanged)
  {
    if (!m_bAnimTimerInFlight && GetPropertyAnimDocument()->GetPlayAnimation())
    {
      m_bAnimTimerInFlight = true;
      QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
    }

    m_pScrubberToolbar->SetButtonState(GetPropertyAnimDocument()->GetPlayAnimation(), GetPropertyAnimDocument()->GetRepeatAnimation());
  }
}

void WQtPropertyAnimAssetDocumentWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  UpdateSelectionData();
}

void WQtPropertyAnimAssetDocumentWindow::UpdateSelectionData()
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  m_MapSelectionToTrack.Clear();
  m_pGradientToDisplay = nullptr;
  m_iMapGradientToTrack = -1;
  m_CurvesToDisplay.Clear();
  m_CurvesToDisplay.m_bOwnsData = false;
  m_CurvesToDisplay.m_uiFramesPerSecond = pDoc->GetProperties()->m_uiFramesPerSecond;

  WSet<WInt32> tracks;

  for (const QModelIndex& selIdx : m_pSelectionModel->selection().indexes())
  {
    WQtPropertyAnimModelTreeEntry* pTreeItem =
      reinterpret_cast<WQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(selIdx, WQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

    WQtPropertyAnimModel* pModel = m_pPropertiesModel;

    auto addRecursive = [&tracks, pModel](auto& ref_self, const WQtPropertyAnimModelTreeEntry* pTreeItem) -> void
    {
      if (pTreeItem->m_pTrack != nullptr)
        tracks.Insert(pTreeItem->m_iTrackIdx);

      for (WInt32 iChild : pTreeItem->m_Children)
      {
        // cannot use 'addRecursive' here, because the name is not yet fully defined
        ref_self(ref_self, &pModel->GetAllEntries()[iChild]);
      }
    };

    addRecursive(addRecursive, pTreeItem);
  }

  auto& trackArray = pDoc->GetProperties()->m_Tracks;
  for (auto it = tracks.GetIterator(); it.IsValid(); ++it)
  {
    const WInt32 iTrackIdx = it.Key();

    // this can happen during undo/redo when the selection still names data that has just been removed
    if (iTrackIdx >= (WInt32)trackArray.GetCount())
      continue;

    if (trackArray[iTrackIdx]->m_Target != WPropertyAnimTarget::Color)
    {
      m_MapSelectionToTrack.PushBack(iTrackIdx);

      m_CurvesToDisplay.m_Curves.PushBack(&trackArray[iTrackIdx]->m_FloatCurve);
    }
    else
    {
      m_pGradientToDisplay = &trackArray[iTrackIdx]->m_ColorGradient;
      m_iMapGradientToTrack = iTrackIdx;
    }
  }

  m_pCurveEditor->ClearSelection();

  UpdateCurveEditor();
  UpdateGradientEditor();

  if (m_pEventTrackPanel->hasFocus() || m_pEventTrackEditor->hasFocus() || m_pEventTrackEditor->EventTrackEdit->hasFocus())
  {
  }
  else if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pColorGradientPanel->raise();
  }
}

void WQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged(WUInt64 uiTick)
{
  GetPropertyAnimDocument()->SetScrubberPosition(uiTick);
}

void WQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  pHistory->StartTransaction("Delete Tracks");

  m_pGradientToDisplay = nullptr;
  m_CurvesToDisplay.Clear();

  // delete the tracks with the highest index first, otherwise the lower indices become invalid
  // do this before modifying anything, as m_MapSelectionToTrack will change once the remove commands are executed
  WTempHybridArray<WInt32, 16> sortedTrackIDs;
  {
    for (WInt32 iTrack : m_MapSelectionToTrack)
    {
      sortedTrackIDs.PushBack(iTrack);
    }

    if (m_iMapGradientToTrack >= 0)
    {
      sortedTrackIDs.PushBack(m_iMapGradientToTrack);
    }

    sortedTrackIDs.Sort();
  }

  for (WUInt32 i = sortedTrackIDs.GetCount(); i > 0; --i)
  {
    const WInt32 iTrack = sortedTrackIDs[i - 1];

    const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
    {
      WRemoveObjectCommand cmd;
      cmd.m_Object = trackGuid.Get<WUuid>();

      pHistory->AddCommand(cmd).AssertSuccess();
    }
  }

  m_MapSelectionToTrack.Clear();
  m_iMapGradientToTrack = -1;

  pHistory->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  WTempHybridArray<WUuid, 16> rebindTracks;

  for (WInt32 iTrack : m_MapSelectionToTrack)
  {
    const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<WUuid>());
  }

  if (m_iMapGradientToTrack >= 0)
  {
    const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<WUuid>());
  }

  bool ok = false;
  QString result = QInputDialog::getText(this, "Change Animation Binding", "New Binding Path:", QLineEdit::Normal, "", &ok);

  if (!ok)
    return;

  m_pSelectionModel->clear();

  WStringBuilder path = result.toUtf8().data();
  ;
  path.MakeCleanPath();
  const WVariant varRes = path.GetData();

  pHistory->StartTransaction("Rebind Tracks");

  for (const WUuid guid : rebindTracks)
  {
    WSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = guid;

    cmdSet.m_sProperty = "ObjectPath";
    cmdSet.m_NewValue = varRes;
    pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
  }

  pHistory->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onPlaybackTick()
{
  m_bAnimTimerInFlight = false;

  if (!GetPropertyAnimDocument()->GetPlayAnimation())
    return;

  GetPropertyAnimDocument()->ExecuteAnimationPlaybackStep();

  m_bAnimTimerInFlight = true;
  QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
}

void WQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked()
{
  GetPropertyAnimDocument()->SetPlayAnimation(!GetPropertyAnimDocument()->GetPlayAnimation());
}

void WQtPropertyAnimAssetDocumentWindow::onRepeatClicked()
{
  GetPropertyAnimDocument()->SetRepeatAnimation(!GetPropertyAnimDocument()->GetRepeatAnimation());
}

void WQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked()
{
  GetPropertyAnimDocument()->AdjustDuration();
}

void WQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent(double duration)
{
  GetPropertyAnimDocument()->SetAnimationDurationTicks((WUInt64)(duration * 4800.0));
}

void WQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked(const QModelIndex& index)
{
  WQtPropertyAnimModelTreeEntry* pTreeItem =
    reinterpret_cast<WQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(index, WQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

  if (pTreeItem != nullptr && pTreeItem->m_pTrack != nullptr)
  {
    if (pTreeItem->m_pTrack->m_Target == WPropertyAnimTarget::Color)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
    else
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
  }
  else
  {
    if (!m_CurvesToDisplay.m_Curves.IsEmpty())
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
    else if (m_pGradientToDisplay != nullptr)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
  }
}

void WQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks()
{
  if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurveEditor->FrameCurve();
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pGradientEditor->FrameGradient();
    m_pColorGradientPanel->raise();
  }
}

WPropertyAnimAssetDocument* WQtPropertyAnimAssetDocumentWindow::GetPropertyAnimDocument()
{
  return static_cast<WPropertyAnimAssetDocument*>(GetDocument());
}

// void WQtPropertyAnimAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
//{
//  if (static_cast<WPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pObject, e.m_sProperty))
//    return;
//
//  // TODO: only update what needs to be updated
//
//  //m_bUpdateEventTrackEditor = true;
//  //m_bUpdateCurveEditor = true;
//  //m_bUpdateGradientEditor = true;
//}

void WQtPropertyAnimAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent &&
      static_cast<WPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent &&
      static_cast<WPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdateSelectionData();
      break;

    default:
      break;
  }
}


void WQtPropertyAnimAssetDocumentWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  // this would show the document properties
  // if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  //{
  //  // delayed execution
  //  QTimer::singleShot(1, [this]()
  //  {
  //    GetDocument()->GetSelectionManager()->SetSelection(GetPropertyAnimDocument()->GetPropertyObject());
  //  });
  //}
}


void WQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  if (e.m_Type == WCommandHistoryEvent::Type::TransactionEnded || e.m_Type == WCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == WCommandHistoryEvent::Type::RedoEnded)
  {
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
}

void WQtPropertyAnimAssetDocumentWindow::UpdateCurveEditor()
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pCurveEditor->SetCurveExtents(0, pDoc->GetAnimationDurationTime().GetSeconds(), true, true);
  m_pCurveEditor->SetCurves(m_CurvesToDisplay);
}


void WQtPropertyAnimAssetDocumentWindow::UpdateGradientEditor()
{
  if (m_pGradientToDisplay == nullptr || m_iMapGradientToTrack < 0)
  {
    // TODO: clear gradient editor ?
    WColorGradient empty;
    m_pGradientEditor->SetColorGradient(empty);
  }
  else
  {
    WColorGradient gradient;
    m_pGradientToDisplay->FillGradientData(gradient);
    m_pGradientEditor->SetColorGradient(gradient);
  }
}


void WQtPropertyAnimAssetDocumentWindow::UpdateEventTrackEditor()
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pEventTrackEditor->SetData(GetPropertyAnimDocument()->GetProperties()->m_EventTrack, pDoc->GetAnimationDurationTime().GetSeconds());
}

void WQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void WQtPropertyAnimAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void WQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void WQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void WQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt(WUInt32 uiCurveIdx, WInt64 tickX, double clickPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  pDoc->InsertCurveCpAt(trackGuid.Get<WUuid>(), tickX, clickPosY);
}

void WQtPropertyAnimAssetDocumentWindow::onCurveCpMoved(WUInt32 uiCurveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  iTickX = WMath::Max<WInt64>(iTickX, 0);

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted(WUInt32 uiCurveIdx, WUInt32 cpIdx)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved(WUInt32 uiCurveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = WMath::Max(newPosX, 0.0f);
  else
    newPosX = WMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue = WVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents(WUInt32 uiCurveIdx, WUInt32 cpIdx, bool bLink)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<WUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const WVec2 leftTangent = pDoc->GetProperties()->m_Tracks[iTrackIdx]->m_FloatCurve.m_ControlPoints[cpIdx].m_LeftTangent;
    const WVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(uiCurveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void WQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged(WUInt32 uiCurveIdx, WUInt32 cpIdx, bool rightTangent, int mode)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const WInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<WUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void WQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded(double posX, const WColorGammaUB& color)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  WInt64 tickX = WColorGradient::TimeToTick(posX);
  pDoc->InsertGradientColorCpAt(trackGuid.Get<WUuid>(), tickX, color);
}


void WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded(double posX, WUInt8 alpha)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  WInt64 tickX = WColorGradient::TimeToTick(posX);
  pDoc->InsertGradientAlphaCpAt(trackGuid.Get<WUuid>(), tickX, alpha);
}


void WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  WInt64 tickX = WColorGradient::TimeToTick(posX);
  pDoc->InsertGradientIntensityCpAt(trackGuid.Get<WUuid>(), tickX, intensity);
}

void WQtPropertyAnimAssetDocumentWindow::MoveGradientCP(WInt32 idx, double newPosX, const char* szArrayName)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  WVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = WColorGradient::TimeToTick(newPosX);
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved(WInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "ColorCPs");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved(WInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "AlphaCPs");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved(WInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "IntensityCPs");
}

void WQtPropertyAnimAssetDocumentWindow::RemoveGradientCP(WInt32 idx, const char* szArrayName)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  WVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted(WInt32 idx)
{
  RemoveGradientCP(idx, "ColorCPs");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted(WInt32 idx)
{
  RemoveGradientCP(idx, "AlphaCPs");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted(WInt32 idx)
{
  RemoveGradientCP(idx, "IntensityCPs");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged(WInt32 idx, const WColorGammaUB& color)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  WVariant objGuid = gradientObject->GetTypeAccessor().GetValue("ColorCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void WQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged(WInt32 idx, WUInt8 alpha)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  WVariant objGuid = gradientObject->GetTypeAccessor().GetValue("AlphaCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged(WInt32 idx, float intensity)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const WVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const WDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<WUuid>());
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  WVariant objGuid = gradientObject->GetTypeAccessor().GetValue("IntensityCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation()
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}

void WQtPropertyAnimAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt(WInt64 tickX, QString value)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved(WUInt32 cpIdx, WInt64 iTickX)
{
  iTickX = WMath::Max<WInt64>(iTickX, 0);

  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  WObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const WUuid trackGuid = accessor.Get<WUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const WDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const WVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted(WUInt32 cpIdx)
{
  WPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  WObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const WUuid trackGuid = accessor.Get<WUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const WDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const WVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void WQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

//////////////////////////////////////////////////////////////////////////

WQtPropertyAnimAssetTreeView::WQtPropertyAnimAssetTreeView(QWidget* pParent)
  : QTreeView(pParent)
{
  setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
}

void WQtPropertyAnimAssetTreeView::initialize()
{
  connect(model(), &QAbstractItemModel::modelAboutToBeReset, this, &WQtPropertyAnimAssetTreeView::onBeforeModelReset);
  connect(model(), &QAbstractItemModel::modelReset, this, &WQtPropertyAnimAssetTreeView::onAfterModelReset);
}

void WQtPropertyAnimAssetTreeView::storeExpandState(const QModelIndex& parent)
{
  const QAbstractItemModel* pModel = model();

  const WUInt32 numRows = pModel->rowCount(parent);
  for (WUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    const bool expanded = isExpanded(idx);

    QString path = pModel->data(idx, WQtPropertyAnimModel::UserRoles::Path).toString();

    if (!expanded)
      m_NotExpandedState.insert(path);

    storeExpandState(idx);
  }
}

void WQtPropertyAnimAssetTreeView::restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection)
{
  const QAbstractItemModel* pModel = model();

  const WUInt32 numRows = pModel->rowCount(parent);
  for (WUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    QString path = pModel->data(idx, WQtPropertyAnimModel::UserRoles::Path).toString();

    const bool notExpanded = m_NotExpandedState.contains(path);

    if (!notExpanded)
      setExpanded(idx, true);

    if (m_SelectedItems.contains(path))
      newSelection.append(idx);

    restoreExpandState(idx, newSelection);
  }
}

void WQtPropertyAnimAssetTreeView::onBeforeModelReset()
{
  m_NotExpandedState.clear();
  m_SelectedItems.clear();

  storeExpandState(QModelIndex());

  const QAbstractItemModel* pModel = model();

  for (QModelIndex idx : selectionModel()->selectedRows())
  {
    QString path = pModel->data(idx, WQtPropertyAnimModel::UserRoles::Path).toString();
    m_SelectedItems.insert(path);
  }
}

void WQtPropertyAnimAssetTreeView::onAfterModelReset()
{
  QModelIndexList newSelection;
  restoreExpandState(QModelIndex(), newSelection);

  // changing the selection is not possible in onAfterModelReset, probably because the items are not yet fully valid
  // has to be done shortly after
  QTimer::singleShot(0, this, [this, newSelection]()
    {
    selectionModel()->clearSelection();
    for (const auto& idx : newSelection)
    {
      selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
    } });
}

void WQtPropertyAnimAssetTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    Q_EMIT DeleteSelectedItemsEvent();
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

void WQtPropertyAnimAssetTreeView::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu m;
  m.setToolTipsVisible(true);
  QAction* pFrameAction = m.addAction("Frame Curve");
  QAction* pRemoveAction = m.addAction("Remove Track");
  QAction* pBindingAction = m.addAction("Change Binding...");
  m.setDefaultAction(pFrameAction);

  pRemoveAction->setShortcut(Qt::Key_Delete);

  connect(pFrameAction, &QAction::triggered, this, [this](bool)
    { Q_EMIT FrameSelectedItemsEvent(); });
  connect(pRemoveAction, &QAction::triggered, this, [this](bool)
    { Q_EMIT DeleteSelectedItemsEvent(); });
  connect(pBindingAction, &QAction::triggered, this, [this](bool)
    { Q_EMIT RebindSelectedItemsEvent(); });

  m.exec(QCursor::pos());
}

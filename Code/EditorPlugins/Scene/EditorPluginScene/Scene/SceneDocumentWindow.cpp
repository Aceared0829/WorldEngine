#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtSceneDocumentWindow::WQtSceneDocumentWindow(WSceneDocument* pDocument)
  : WQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](WQtEngineDocumentWindow* pWindow, WEngineViewConfig* pConfig) -> WQtEngineViewWidget*
  {
    WQtSceneViewWidget* pWidget = new WQtSceneViewWidget(nullptr, static_cast<WQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new WQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](WGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<WGameObjectDocument*>(GetDocument()), this, this); });

  {
    WQtDocumentPanel* pViewPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pViewPanel->setObjectName("WQtDocumentPanel");
    pViewPanel->setWindowTitle("3D View");
    pViewPanel->setWidget(m_pQuadViewWidget);

    m_pDockManager->setCentralWidget(pViewPanel);
  }

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  SetTargetFramerate(pPreferences->GetMaxFramerate());

  {
    // Menu Bar
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  // Exposed Parameters
  if (GetSceneDocument()->IsPrefab())
  {
    WQtDocumentPanel* pPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPanel->setObjectName("PrefabSettingsPanel");
    pPanel->setWindowTitle("Prefab Settings");

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPanel, pDocument, false);
    WDeque<const WDocumentObject*> selection;
    selection.PushBack(pDocument->GetSettingsObject());
    pPropertyGrid->SetSelection(selection);
    pPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
  }

  // Properties
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");

    WQtDocumentPanel* pPanelTree = new WQtScenegraphPanel(GetContainerWindow()->GetDockManager(), this, static_cast<WSceneDocument*>(pDocument));
    pPanelTree->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    W_VERIFY(connect(pPropertyGrid, &WQtPropertyGridWidget::ExtendContextMenu, this, &WQtSceneDocumentWindow::ExtendPropertyGridContextMenu), "");

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }

  // If prefab: expand scenegraph and select the root object, as that is the most likely one to be edited.
  if (pDocument->IsPrefab()) {
    pDocument->TriggerExpandScenegraph();
    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

WQtSceneDocumentWindow::~WQtSceneDocumentWindow() = default;

WQtSceneDocumentWindowBase::WQtSceneDocumentWindowBase(WSceneDocument* pDocument)
  : WQtGameObjectDocumentWindow(pDocument)
{
  const WSceneDocument* pSceneDoc = static_cast<const WSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

WQtSceneDocumentWindowBase::~WQtSceneDocumentWindowBase()
{
  GetSceneDocument()->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

WSceneDocument* WQtSceneDocumentWindowBase::GetSceneDocument() const
{
  return static_cast<WSceneDocument*>(GetDocument());
}

void WQtSceneDocumentWindowBase::CreateImageCapture(const char* szOutputPath)
{
  m_pQuadViewWidget->GetActiveMainViews()[0]->GetViewWidget()->TakeScreenshot(szOutputPath);
}

void WQtSceneDocumentWindowBase::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


WObjectAccessorBase* WQtSceneDocumentWindowBase::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool WQtSceneDocumentWindowBase::CanDuplicateSelection() const
{
  return true;
}

void WQtSceneDocumentWindowBase::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void WQtSceneDocumentWindowBase::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = WSnapProvider::GetTranslationSnapValue();

  if (fSnap == 0.0f)
    return;

  const WDeque<const WDocumentObject*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
  if (selection.IsEmpty())
    return;

  const auto& pivotObj = selection.PeekBack();

  WVec3 vPivotSnapOffset;

  if (!bSnapEachObject)
  {
    // if we snap by the pivot object only, the last selected object must be a valid game object
    if (!pivotObj->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
      return;

    const WVec3 vPivotPos = GetSceneDocument()->GetGlobalTransform(pivotObj).m_vPosition;
    WVec3 vSnappedPos = vPivotPos;
    WSnapProvider::SnapTranslation(vSnappedPos);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  WDeque<WSelectedGameObject> gizmoSelection;
  GetGameObjectDocument()->ComputeTopLevelSelectedGameObjects(gizmoSelection);

  if (gizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (WUInt32 sel = 0; sel < gizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = gizmoSelection[sel];

    WTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition = obj.m_GlobalTransform.m_vPosition;
      WSnapProvider::SnapTranslation(vSnappedPos.m_vPosition);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos, TransformationChanges::Translation);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  gizmoSelection.Clear();

  ShowTemporaryStatusBarMsg(WFmt("Snap to Grid ({})", bSnapEachObject ? "Each Object" : "Pivot"));
}

void WQtSceneDocumentWindowBase::GameObjectEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      // Focus is done by WQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(WSceneDocument::ShowOrHide::Show);
      break;

    case WGameObjectEvent::Type::TriggerFocusOnSelection_All:
      // Focus is done by WQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(WSceneDocument::ShowOrHide::Show);
      break;

    case WGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid:
      SnapSelectionToPosition(false);
      break;

    case WGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid:
      SnapSelectionToPosition(true);
      break;

    default:
      break;
  }
}

void WQtSceneDocumentWindowBase::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  WSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtSceneDocumentWindowBase::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    WSimulationSettingsMsgToEngine msg;
    auto pSceneDoc = GetSceneDocument();
    msg.m_bSimulateWorld = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed = pSceneDoc->GetPauseSimulation() ? 0.0f : pSceneDoc->GetSimulationSpeed();

    if (msg.m_bSimulateWorld && pSceneDoc->GetStepSimulation())
    {
      msg.m_fSimulationSpeed = 1.0f;
      pSceneDoc->SetStepSimulation(false);
    }

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
    pView->SetPickTransparent(GetGameObjectDocument()->GetPickTransparent());
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void WQtSceneDocumentWindowBase::ExtendPropertyGridContextMenu(QMenu& menu, WQtPropertyWidget* pPropWidget)
{
  if (!GetSceneDocument()->IsPrefab())
    return;

  const WRTTI* pType = pPropWidget->GetType();
  const WAbstractProperty* pProp = pPropWidget->GetProperty();
  WObjectAccessorBase* pAccessor = pPropWidget->GetObjectAccessor();
  const auto& items = pPropWidget->GetSelection();

  WUInt32 iExposed = 0;
  for (WUInt32 i = 0; i < items.GetCount(); i++)
  {
    WInt32 index = GetSceneDocument()->FindExposedParameter(pAccessor, items[i].m_pObject, pType, pProp, items[i].m_Index);
    if (index != -1)
      iExposed++;
  }
  menu.addSeparator();
  {
    QAction* pAction = menu.addAction("Expose as Parameter");
    pAction->setEnabled(iExposed < items.GetCount());
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pAccessor, pType, pProp]()
      {
      while (true)
      {
        bool bOk = false;
        QString name = QInputDialog::getText(this, "Parameter Name", "Name:", QLineEdit::Normal, pProp->GetPropertyName(), &bOk);

        if (!bOk)
          return;

        if (!WStringUtils::IsValidIdentifierName(name.toUtf8().data()))
        {
          WQtUiServices::GetSingleton()->MessageBoxInformation("This name is not a valid identifier.\nAllowed characters are a-z, A-Z, "
                                                                "0-9 and _.\nWhitespace and special characters are not allowed.");
          continue; // try again
        }

        pAccessor->StartTransaction("Expose as Parameter");
        for (const WPropertySelection& sel : items)
        {
          WInt32 index = GetSceneDocument()->FindExposedParameter(pAccessor, sel.m_pObject, pType, pProp, sel.m_Index);
          if (index == -1)
          {
            GetSceneDocument()->AddExposedParameter(name.toUtf8(), pAccessor, sel.m_pObject, pType, pProp, sel.m_Index).LogFailure();
          }
        }
        pAccessor->FinishTransaction();
        return;
      } });
  }
  {
    QAction* pAction = menu.addAction("Remove Exposed Parameter");
    pAction->setEnabled(iExposed > 0);
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pAccessor, pType, pProp]()
      {
      pAccessor->StartTransaction("Remove Exposed Parameter");
      for (const WPropertySelection& sel : items)
      {
        WInt32 index = GetSceneDocument()->FindExposedParameter(pAccessor, sel.m_pObject, pType, pProp, sel.m_Index);
        if (index != -1)
        {
          GetSceneDocument()->RemoveExposedParameter(index).LogFailure();
        }
      }
      pAccessor->FinishTransaction(); });
  }
}

void WQtSceneDocumentWindowBase::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg)
{
  WQtGameObjectDocumentWindow::ProcessMessageEventHandler(pMsg);
}

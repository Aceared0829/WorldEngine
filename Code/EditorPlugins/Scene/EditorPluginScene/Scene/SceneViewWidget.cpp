#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QKeyEvent>

bool WQtSceneViewWidget::s_bContextMenuInitialized = false;

WQtSceneViewWidget::WQtSceneViewWidget(QWidget* pParent, WQtGameObjectDocumentWindow* pOwnerWindow, WEngineViewConfig* pViewConfig)
  : WQtGameObjectViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  if (WDynamicCast<WScene2Document*>(pOwnerWindow->GetDocument()))
  {
    // #TODO Not the cleanest solution but this replaces the default selection context of the base class.
    const WUInt32 uiSelectionIndex = m_InputContexts.IndexOf(m_pSelectionContext);
    W_DEFAULT_DELETE(m_pSelectionContext);
    m_pSelectionContext = W_DEFAULT_NEW(WSceneSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts[uiSelectionIndex] = m_pSelectionContext;
  }
}

WQtSceneViewWidget::~WQtSceneViewWidget() = default;

bool WQtSceneViewWidget::IsPickingAgainstSelectionAllowed() const
{
  if (m_bInDragAndDropOperation && m_bAllowPickSelectedWhileDragging)
  {
    return true;
  }

  return WQtEngineViewWidget::IsPickingAgainstSelectionAllowed();
}

void WQtSceneViewWidget::OnOpenContextMenu(QPoint globalPos)
{
  if (!s_bContextMenuInitialized)
  {
    s_bContextMenuInitialized = true;

    WActionMapManager::RegisterActionMap("SceneViewContextMenu");

    WGameObjectSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    WSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    WEditActions::MapViewContextMenuActions("SceneViewContextMenu");
    WSceneActions::MapViewContextMenuActions("SceneViewContextMenu");
  }

  {
    WQtMenuActionMapView menu(nullptr);

    WActionContext context;
    context.m_sMapping = "SceneViewContextMenu";
    context.m_pDocument = GetDocumentWindow()->GetDocument();
    context.m_pWindow = this;
    menu.SetActionContext(context);

    menu.exec(globalPos);
  }
}

void WQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  WQtEngineViewWidget::dragEnterEvent(e);

  // can only drag & drop objects around in perspective mode
  // when dragging between two windows, the editor crashes
  // can be reproduced with two perspective windows as well
  // if (m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
  // return;

  m_LastDragMoveEvent = WTime::Now();
  m_bAllowPickSelectedWhileDragging = false;

  {
    const QPoint screenPos = e->position().toPoint();
    WObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

    WDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    if (WGameObjectDocument* pSceneDoc = WDynamicCast<WGameObjectDocument*>(m_pDocumentWindow->GetDocument()))
    {
      pSceneDoc = pSceneDoc->GetRedirectedGameObjectDoc();
      const WUuid guid = pSceneDoc->GetActiveParent();

      // the object may not exist anymore
      if (pSceneDoc->GetObjectManager()->GetObject(guid) != nullptr)
      {
        info.m_ActiveParentObject = guid;
      }
    }

    WDragDropConfig cfg;
    if (WDragDropHandler::BeginDragDropOperation(&info, &cfg))
    {
      m_bAllowPickSelectedWhileDragging = cfg.m_bPickSelectedObjects;

      e->acceptProposedAction();
      return;
    }
  }

  m_bInDragAndDropOperation = false;
}

void WQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  WDragDropHandler::CancelDragDrop();

  WQtEngineViewWidget::dragLeaveEvent(e);
}

void WQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  const WTime tNow = WTime::Now();

  if (tNow - m_LastDragMoveEvent < WTime::MakeFromSeconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  if (WDragDropHandler::IsHandlerActive())
  {
    const QPoint screenPos = e->position().toPoint();
    WObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

    WDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    WDragDropHandler::UpdateDragDropOperation(&info);
  }
}

void WQtSceneViewWidget::dropEvent(QDropEvent* e)
{
  if (WDragDropHandler::IsHandlerActive())
  {
    const QPoint screenPos = e->position().toPoint();
    WObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

    WDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    WDragDropHandler::FinishDragDrop(&info);

    setFocus();
  }

  WQtEngineViewWidget::dropEvent(e);
}

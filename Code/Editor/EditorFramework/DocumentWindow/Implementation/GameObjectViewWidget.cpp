#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

WQtGameObjectViewWidget::WQtGameObjectViewWidget(QWidget* pParent, WQtGameObjectDocumentWindow* pOwnerWindow, WEngineViewConfig* pViewConfig)
  : WQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  m_pSelectionContext = W_DEFAULT_NEW(WSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = W_DEFAULT_NEW(WCameraMoveContext, pOwnerWindow, this);
  m_pOrthoGizmoContext = W_DEFAULT_NEW(WOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pCameraMoveContext->LoadState();

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pOrthoGizmoContext);
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

WQtGameObjectViewWidget::~WQtGameObjectViewWidget()
{
  W_DEFAULT_DELETE(m_pOrthoGizmoContext);
  W_DEFAULT_DELETE(m_pSelectionContext);
  W_DEFAULT_DELETE(m_pCameraMoveContext);
}

void WQtGameObjectViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(WVec2I32(width(), height()));

  WQtEngineViewWidget::SyncToEngine();
}

void WQtGameObjectViewWidget::HandleMarqueePickingResult(const WViewMarqueePickingResultMsgToEditor* pMsg)
{
  auto pSelMan = GetDocumentWindow()->GetDocument()->GetSelectionManager();
  auto pObjMan = GetDocumentWindow()->GetDocument()->GetObjectManager();

  if (m_uiLastMarqueeActionID != pMsg->m_uiActionIdentifier)
  {
    m_uiLastMarqueeActionID = pMsg->m_uiActionIdentifier;

    m_MarqueeBaseSelection.Clear();

    if (pMsg->m_uiWhatToDo == 0) // set selection
      pSelMan->Clear();

    const auto& curSel = pSelMan->GetSelection();
    for (auto pObj : curSel)
    {
      m_MarqueeBaseSelection.PushBack(pObj->GetGuid());
    }
  }

  WDeque<const WDocumentObject*> newSelection;

  for (WUuid guid : m_MarqueeBaseSelection)
  {
    auto pObject = pObjMan->GetObject(guid);
    newSelection.PushBack(pObject);
  }

  for (WUuid guid : pMsg->m_ObjectGuids)
  {
    const WDocumentObject* pObject = pObjMan->GetObject(guid);

    if (pMsg->m_uiWhatToDo == 2) // remove from selection
    {
      // keep selection order
      newSelection.RemoveAndCopy(pObject);
    }
    else // add/set selection
    {
      if (!newSelection.Contains(pObject))
        newSelection.PushBack(pObject);
    }
  }

  pSelMan->SetSelection(newSelection);
}

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/VisualGraph/CommentNode.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>

WRttiMappedObjectFactory<WQtVisualGraphNode> WQtVisualGraphScene::s_NodeFactory;
WRttiMappedObjectFactory<WQtVisualGraphPin> WQtVisualGraphScene::s_PinFactory;
WRttiMappedObjectFactory<WQtVisualGraphConnection> WQtVisualGraphScene::s_ConnectionFactory;

WVec2 WQtVisualGraphScene::s_vLastMouseInteraction(0);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualGraphComment)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(WGetStaticRTTI<WVisualGraphComment>(), [](const WRTTI* pRtti) -> WQtVisualGraphNode*
      { return new WQtVisualGraphCommentNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(WGetStaticRTTI<WVisualGraphComment>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WQtVisualGraphScene::WQtVisualGraphScene(QObject* pParent)
  : QGraphicsScene(pParent)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(this, &QGraphicsScene::selectionChanged, this, &WQtVisualGraphScene::OnSelectionChanged);
}

WQtVisualGraphScene::~WQtVisualGraphScene()
{
  disconnect(this, &QGraphicsScene::selectionChanged, this, &WQtVisualGraphScene::OnSelectionChanged);

  Clear();

  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(WMakeDelegate(&WQtVisualGraphScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtVisualGraphScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtVisualGraphScene::PropertyEventsHandler, this));
  }
}

void WQtVisualGraphScene::InitScene(const WVisualGraphObjectManager* pManager)
{
  W_ASSERT_DEV(pManager != nullptr, "Invalid node manager");

  m_pManager = pManager;

  m_pManager->m_NodeEvents.AddEventHandler(WMakeDelegate(&WQtVisualGraphScene::NodeEventsHandler, this));
  m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtVisualGraphScene::SelectionEventsHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtVisualGraphScene::PropertyEventsHandler, this));

  // Create Nodes
  const auto& rootObjects = pManager->GetRootObject()->GetChildren();
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsNode(pObject) || pManager->IsComment(pObject))
    {
      CreateQtNode(pObject);
    }
  }
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsConnection(pObject))
    {
      CreateQtConnection(pObject);
    }
  }
}

const WDocument* WQtVisualGraphScene::GetDocument() const
{
  return m_pManager->GetDocument();
}

const WVisualGraphObjectManager* WQtVisualGraphScene::GetDocumentNodeManager() const
{
  return m_pManager;
}

WRttiMappedObjectFactory<WQtVisualGraphNode>& WQtVisualGraphScene::GetNodeFactory()
{
  return s_NodeFactory;
}

WRttiMappedObjectFactory<WQtVisualGraphPin>& WQtVisualGraphScene::GetPinFactory()
{
  return s_PinFactory;
}

WRttiMappedObjectFactory<WQtVisualGraphConnection>& WQtVisualGraphScene::GetConnectionFactory()
{
  return s_ConnectionFactory;
}

void WQtVisualGraphScene::SetConnectionStyle(WEnum<ConnectionStyle> style)
{
  m_ConnectionStyle = style;
  invalidate();
}

void WQtVisualGraphScene::SetConnectionDecorationFlags(WBitflags<ConnectionDecorationFlags> flags)
{
  m_ConnectionDecorationFlags = flags;
  invalidate();
}

void WQtVisualGraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  m_vMousePos = WVec2(event->scenePos().x(), event->scenePos().y());
  s_vLastMouseInteraction = m_vMousePos;

  if (m_pTempConnection)
  {
    event->accept();

    WVec2 bestPos = m_vMousePos;

    // snap to the closest pin that we can connect to
    if (!m_ConnectablePins.IsEmpty())
    {
      const float fPinSize = m_ConnectablePins[0]->sceneBoundingRect().height();

      // this is also the threshold at which we snap to another position
      float fDistToBest = WMath::Square(fPinSize * 2.5f);

      for (auto pin : m_ConnectablePins)
      {
        const QPointF center = pin->sceneBoundingRect().center();
        const WVec2 pt = WVec2(center.x(), center.y());
        const float lenSqr = (pt - s_vLastMouseInteraction).GetLengthSquared();

        if (lenSqr < fDistToBest)
        {
          fDistToBest = lenSqr;
          bestPos = pt;
        }
      }
    }

    if (m_pStartPin->GetPin()->GetType() == WVisualGraphPin::Type::Input)
    {
      m_pTempConnection->SetPosOut(QPointF(bestPos.x, bestPos.y));
    }
    else
    {
      m_pTempConnection->SetPosIn(QPointF(bestPos.x, bestPos.y));
    }
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);
}

void WQtVisualGraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  switch (event->button())
  {
    case Qt::LeftButton:
    {
      QList<QGraphicsItem*> itemList = items(event->scenePos(), Qt::IntersectsItemBoundingRect);
      for (QGraphicsItem* item : itemList)
      {
        if (item->type() != Type::Pin)
          continue;

        event->accept();
        WQtVisualGraphPin* pPin = static_cast<WQtVisualGraphPin*>(item);
        m_pStartPin = pPin;
        m_pTempConnection = new WQtVisualGraphConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());

        if (pPin->GetPin()->GetType() == WVisualGraphPin::Type::Input)
        {
          m_pTempConnection->SetDirIn(pPin->GetPinDir());
          m_pTempConnection->SetDirOut(-pPin->GetPinDir());
        }
        else
        {
          m_pTempConnection->SetDirIn(-pPin->GetPinDir());
          m_pTempConnection->SetDirOut(pPin->GetPinDir());
        }

        MarkupConnectablePins(pPin);
        return;
      }
    }
    break;
    case Qt::RightButton:
    {
      event->accept();
      return;
    }

    default:
      break;
  }

  QGraphicsScene::mousePressEvent(event);
}

void WQtVisualGraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();

    const bool startWasInput = m_pStartPin->GetPin()->GetType() == WVisualGraphPin::Type::Input;
    const QPointF releasePos = startWasInput ? m_pTempConnection->GetOutPos() : m_pTempConnection->GetInPos();

    QList<QGraphicsItem*> itemList = items(releasePos, Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem* item : itemList)
    {
      if (item->type() != Type::Pin)
        continue;

      WQtVisualGraphPin* pPin = static_cast<WQtVisualGraphPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const WVisualGraphPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const WVisualGraphPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ConnectPinsAction(*pSourcePin, *pTargetPin);
        goto cleanup;
      }
    }

    OpenSearchMenu(QCursor::pos());

    if (m_pTempNode)
    {
      const auto Pins = startWasInput ? m_pTempNode->GetOutputPins() : m_pTempNode->GetInputPins();

      for (auto& pPin : Pins)
      {
        const WVisualGraphPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const WVisualGraphPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        WVisualGraphObjectManager::CanConnectResult connect;
        WStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), *pSourcePin, *pTargetPin, connect);
        if (res.Succeeded())
        {
          ConnectPinsAction(*pSourcePin, *pTargetPin);
          break;
        }
      }
    }

  cleanup:
    delete m_pTempConnection;
    m_pTempConnection = nullptr;
    m_pStartPin = nullptr;
    m_pTempNode = nullptr;

    ResetConnectablePinMarkup();
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  WSet<const WDocumentObject*> moved;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetFlags().IsSet(WQtVisualGraphNodeFlags::Moved))
    {
      moved.Insert(it.Key());
      it.Value()->ResetFlags();
    }
  }

  if (!moved.IsEmpty())
  {
    WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Move Node");

    WStatus res(W_SUCCESS);
    for (auto pObject : moved)
    {
      WMoveNodeCommand move;
      move.m_Object = pObject->GetGuid();
      auto pos = m_Nodes[pObject]->pos();
      move.m_NewPos = WVec2(pos.x(), pos.y());
      res = history->AddCommand(move);
      if (res.Failed())
        break;
    }

    if (res.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Move node failed");
  }
}

void WQtVisualGraphScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
{
  QTransform id;

  QGraphicsItem* pItem = itemAt(contextMenuEvent->scenePos(), id);
  int iType = pItem != nullptr ? pItem->type() : -1;
  while (pItem && !(iType >= Type::Node && iType <= Type::Connection))
  {
    pItem = pItem->parentItem();
    iType = pItem != nullptr ? pItem->type() : -1;
  }

  QMenu menu;
  if (iType == Type::Pin)
  {
    WQtVisualGraphPin* pPin = static_cast<WQtVisualGraphPin*>(pItem);
    QAction* pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked)
      { DisconnectPinsAction(pPin); });

    pPin->ExtendContextMenu(menu);
  }
  else if (iType == Type::Node)
  {
    WQtVisualGraphNode* pNode = static_cast<WQtVisualGraphNode*>(pItem);

    // if we clicked on an unselected item, make it the only selected item
    if (!pNode->isSelected())
    {
      clearSelection();
      pNode->setSelected(true);
    }

    // Delete Node
    {
      QAction* pAction = new QAction("Remove", &menu);
      pAction->setShortcut(QKeySequence("Del"));
      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked)
        { RemoveSelectedNodesAction(); });
    }

    // Frame Content
    {
      QAction* pAction = new QAction("Frame", &menu);
      pAction->setShortcut(QKeySequence("F"));

      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked)
        {
          QList<QGraphicsView*> viewList = views();
          if (!viewList.isEmpty())
          {
            if (WQtVisualGraphView* pView = qobject_cast<WQtVisualGraphView*>(viewList.first()))
              pView->FrameContent();
          }
          //
        });
    }

    // Add Comment
    {
      QAction* pAction = new QAction("Add Comment", &menu);
      pAction->setShortcut(QKeySequence("C"));
      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked)
        { AddCommentAroundSelectionAction(); });
    }

    pNode->ExtendContextMenu(menu);
  }
  else if (iType == Type::Connection)
  {
    WQtVisualGraphConnection* pConnection = static_cast<WQtVisualGraphConnection*>(pItem);
    QAction* pAction = new QAction("Delete Connection", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pConnection](bool bChecked)
      { DisconnectPinsAction(pConnection); });

    pConnection->ExtendContextMenu(menu);
  }
  else
  {
    OpenSearchMenu(contextMenuEvent->screenPos());
    return;
  }

  menu.exec(contextMenuEvent->screenPos());
}

void WQtVisualGraphScene::keyPressEvent(QKeyEvent* event)
{
  QTransform id;
  QGraphicsItem* pItem = itemAt(QPointF(m_vMousePos.x, m_vMousePos.y), id);
  if (pItem && pItem->type() == Type::Pin)
  {
    WQtVisualGraphPin* pin = static_cast<WQtVisualGraphPin*>(pItem);
    if (event->key() == Qt::Key_Delete)
    {
      DisconnectPinsAction(pin);
    }

    pin->keyPressEvent(event);
  }

  if (event->key() == Qt::Key_Delete)
  {
    RemoveSelectedNodesAction();
  }
  else if (event->key() == Qt::Key_Space)
  {
    OpenSearchMenu(QCursor::pos());
  }
  else if (event->key() == Qt::Key_C && event->modifiers() == Qt::NoModifier)
  {
    AddCommentAroundSelectionAction();
  }

  // Pass Shortcuts/KeyPresses up the chain again, so e.g. Ctrl+S work even if inside a window
  if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
  {
    event->ignore();
  }
}

void WQtVisualGraphScene::Clear()
{
  while (!m_Connections.IsEmpty())
  {
    DeleteQtConnection(m_Connections.GetIterator().Key());
  }

  while (!m_Nodes.IsEmpty())
  {
    DeleteQtNode(m_Nodes.GetIterator().Key());
  }
}

void WQtVisualGraphScene::CreateQtNode(const WDocumentObject* pObject)
{
  WVec2 vPos = m_pManager->GetNodePos(pObject);

  WQtVisualGraphNode* pNode = s_NodeFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pNode == nullptr)
  {
    pNode = new WQtVisualGraphNode();
  }
  m_Nodes[pObject] = pNode;
  addItem(pNode);
  pNode->InitNode(m_pManager, pObject);
  pNode->setPos(vPos.x, vPos.y);

  pNode->ResetFlags();

  // Note: We don't create connections here as it can cause recursion issues
  if (m_pTempConnection)
  {
    m_pTempNode = pNode;
  }
}

void WQtVisualGraphScene::DeleteQtNode(const WDocumentObject* pObject)
{
  WQtVisualGraphNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void WQtVisualGraphScene::CreateQtConnection(const WDocumentObject* pObject)
{
  const WVisualGraphConnection& connection = m_pManager->GetConnection(pObject);
  const WVisualGraphPin& pinSource = connection.GetSourcePin();
  const WVisualGraphPin& pinTarget = connection.GetTargetPin();

  WQtVisualGraphNode* pSource = m_Nodes[pinSource.GetParent()];
  WQtVisualGraphNode* pTarget = m_Nodes[pinTarget.GetParent()];
  WQtVisualGraphPin* pOutput = pSource->GetOutputPin(pinSource);
  WQtVisualGraphPin* pInput = pTarget->GetInputPin(pinTarget);
  W_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  WQtVisualGraphConnection* pQtConnection = s_ConnectionFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new WQtVisualGraphConnection(nullptr);
  }

  addItem(pQtConnection);
  pQtConnection->InitConnection(pObject, &connection);
  pOutput->AddConnection(pQtConnection);
  pInput->AddConnection(pQtConnection);
  m_Connections[pObject] = pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void WQtVisualGraphScene::DeleteQtConnection(const WDocumentObject* pObject)
{
  WQtVisualGraphConnection* pQtConnection = m_Connections[pObject];
  m_Connections.Remove(pObject);

  const WVisualGraphConnection* pConnection = pQtConnection->GetConnection();
  W_ASSERT_DEV(pConnection != nullptr, "No connection");

  const WVisualGraphPin& pinSource = pConnection->GetSourcePin();
  const WVisualGraphPin& pinTarget = pConnection->GetTargetPin();

  WQtVisualGraphNode* pSource = m_Nodes[pinSource.GetParent()];
  WQtVisualGraphNode* pTarget = m_Nodes[pinTarget.GetParent()];
  WQtVisualGraphPin* pOutput = pSource->GetOutputPin(pinSource);
  WQtVisualGraphPin* pInput = pTarget->GetInputPin(pinTarget);
  W_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);

  removeItem(pQtConnection);
  delete pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void WQtVisualGraphScene::RecreateQtPins(const WDocumentObject* pObject)
{
  WQtVisualGraphNode* pNode = m_Nodes[pObject];
  pNode->CreatePins();
  pNode->UpdateState();
  pNode->UpdateGeometry();
}

void WQtVisualGraphScene::CreateNodeObject(const WVisualGraphNodeDesc& nodeTemplate)
{
  WCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Node");

  WStatus res(W_SUCCESS);
  {
    WAddObjectCommand cmd;
    cmd.m_pType = nodeTemplate.m_pType;
    cmd.m_NewObjectGuid = WUuid::MakeUuid();
    cmd.m_Index = -1;

    res = history->AddCommand(cmd);
    if (res.Succeeded())
    {
      WMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = m_vMousePos;
      res = history->AddCommand(move);
    }

    for (auto& propValue : nodeTemplate.m_PropertyValues)
    {
      if (res.Failed())
        break;

      WSetObjectPropertyCommand setCmd;
      setCmd.m_Object = cmd.m_NewObjectGuid;
      setCmd.m_sProperty = propValue.m_sPropertyName.GetString();
      setCmd.m_NewValue = propValue.m_Value;
      res = history->AddCommand(setCmd);
    }
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void WQtVisualGraphScene::NodeEventsHandler(const WVisualGraphObjectManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case WVisualGraphObjectManagerEvent::Type::NodeMoved:
    {
      WVec2 vPos = m_pManager->GetNodePos(e.m_pObject);
      WQtVisualGraphNode* pNode = m_Nodes[e.m_pObject];
      pNode->setPos(vPos.x, vPos.y);
    }
    break;
    case WVisualGraphObjectManagerEvent::Type::AfterPinsConnected:
      CreateQtConnection(e.m_pObject);
      break;

    case WVisualGraphObjectManagerEvent::Type::BeforePinsDisonnected:
      DeleteQtConnection(e.m_pObject);
      break;

    case WVisualGraphObjectManagerEvent::Type::BeforePinsChanged:
      break;

    case WVisualGraphObjectManagerEvent::Type::AfterPinsChanged:
      RecreateQtPins(e.m_pObject);
      break;

    case WVisualGraphObjectManagerEvent::Type::AfterNodeAdded:
      CreateQtNode(e.m_pObject);
      break;

    case WVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved:
      DeleteQtNode(e.m_pObject);
      break;

    default:
      break;
  }
}

void WQtVisualGraphScene::PropertyEventsHandler(const WDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);
  if (it.IsValid())
  {
    it.Value()->ResetFlags();
    it.Value()->update();
  }
}

void WQtVisualGraphScene::SelectionEventsHandler(const WSelectionManagerEvent& e)
{
  const WDeque<const WDocumentObject*>& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;

    clearSelection();

    QList<QGraphicsItem*> qSelection;
    for (const WDocumentObject* pObject : selection)
    {
      auto it = m_Nodes.Find(pObject);
      if (!it.IsValid())
        continue;

      it.Value()->setSelected(true);
    }
    m_bIgnoreSelectionChange = false;
  }

  bool bAnyPaintChanges = false;

  for (auto itCon : m_Connections)
  {
    auto pQtCon = itCon.Value();
    auto pCon = pQtCon->GetConnection();

    const bool prev = pQtCon->m_bAdjacentNodeSelected;

    pQtCon->m_bAdjacentNodeSelected = false;

    for (const WDocumentObject* pObject : selection)
    {
      if (pCon->GetSourcePin().GetParent() == pObject || pCon->GetTargetPin().GetParent() == pObject)
      {
        pQtCon->m_bAdjacentNodeSelected = true;
        break;
      }
    }

    if (prev != pQtCon->m_bAdjacentNodeSelected)
    {
      bAnyPaintChanges = true;
    }
  }

  if (bAnyPaintChanges)
  {
    invalidate();
  }
}

void WQtVisualGraphScene::GetSelectedNodes(WDeque<WQtVisualGraphNode*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == WQtVisualGraphScene::Node)
    {
      WQtVisualGraphNode* pNode = static_cast<WQtVisualGraphNode*>(pItem);
      selection.PushBack(pNode);
    }
  }
}

void WQtVisualGraphScene::MarkupConnectablePins(WQtVisualGraphPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const WRTTI* pConnectionType = m_pManager->GetConnectionType();

  const WVisualGraphPin* pSourcePin = pQtSourcePin->GetPin();
  const bool bConnectForward = pSourcePin->GetType() == WVisualGraphPin::Type::Output;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const WDocumentObject* pDocObject = it.Key();
    WQtVisualGraphNode* pTargetNode = it.Value();

    {
      auto pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        WQtVisualGraphPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);

        WVisualGraphObjectManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pConnectionType, *pSourcePin, *pin, res).IgnoreResult();
        else
          m_pManager->CanConnect(pConnectionType, *pin, *pSourcePin, res).IgnoreResult();

        if (res == WVisualGraphObjectManager::CanConnectResult::ConnectNever)
        {
          pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::CannotConnect);
        }
        else
        {
          m_ConnectablePins.PushBack(pQtTargetPin);

          if (res == WVisualGraphObjectManager::CanConnectResult::Connect1toN || res == WVisualGraphObjectManager::CanConnectResult::ConnectNtoN)
          {
            pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::CanAddConnection);
          }
          else
          {
            pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::CanReplaceConnection);
          }
        }
      }
    }

    {
      auto pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        WQtVisualGraphPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);
        pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::CannotConnectSameDirection);
      }
    }
  }
}

void WQtVisualGraphScene::ResetConnectablePinMarkup()
{
  m_ConnectablePins.Clear();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const WDocumentObject* pDocObject = it.Key();
    WQtVisualGraphNode* pTargetNode = it.Value();

    for (auto& pin : m_pManager->GetInputPins(pDocObject))
    {
      WQtVisualGraphPin* pQtTargetPin = pTargetNode->GetInputPin(*pin);
      pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::None);
    }

    for (auto& pin : m_pManager->GetOutputPins(pDocObject))
    {
      WQtVisualGraphPin* pQtTargetPin = pTargetNode->GetOutputPin(*pin);
      pQtTargetPin->SetHighlightState(WQtVisualGraphPinHighlight::None);
    }
  }
}

void WQtVisualGraphScene::OpenSearchMenu(QPoint screenPos)
{
  if (m_sRecentListName.IsEmpty() && GetDocument() != nullptr)
  {
    // by default every document type gets its own list, so that e.g. visual shader nodes and
    // procedural placement nodes don't share one
    m_sRecentListName = GetDocument()->GetDocumentTypeName();
  }

  QMenu menu;
  WQtSearchableMenu* pSearchMenu = new WQtSearchableMenu(&menu);
  menu.addAction(pSearchMenu);

  connect(pSearchMenu, &WQtSearchableMenu::MenuItemTriggered, this, &WQtVisualGraphScene::OnMenuItemTriggered);
  connect(pSearchMenu, &WQtSearchableMenu::MenuItemTriggered, this, [&menu]()
    { menu.close(); });

  WStringBuilder tmp;
  WStringBuilder sFullPath;

  m_NodeCreationTemplates.Clear();
  m_pManager->GetNodeCreationTemplates(m_NodeCreationTemplates);

  // Add comment node template
  {
    WVisualGraphNodeDesc& commentDesc = m_NodeCreationTemplates.ExpandAndGetRef();
    commentDesc.m_pType = WGetStaticRTTI<WVisualGraphComment>();
    commentDesc.m_sTypeName = "Comment";
    commentDesc.m_sCategory = WMakeHashedString("Misc");
  }

  // the full menu path of each template, used as its identity in the 'recently used' list
  m_NodeCreationTemplatePaths.Clear();
  m_NodeCreationTemplatePaths.SetCount(m_NodeCreationTemplates.GetCount());

  WHybridArray<WString, 64> templateNames;
  templateNames.SetCount(m_NodeCreationTemplates.GetCount());

  for (WUInt32 i = 0; i < m_NodeCreationTemplates.GetCount(); ++i)
  {
    const WVisualGraphNodeDesc& nodeTemplate = m_NodeCreationTemplates[i];
    const WRTTI* pRtti = nodeTemplate.m_pType;
    WStringView sCleanName = nodeTemplate.m_sTypeName.IsEmpty() ? pRtti->GetTypeName() : nodeTemplate.m_sTypeName;

    if (const char* szUnderscore = sCleanName.FindLastSubString("_"))
    {
      sCleanName.SetStartPosition(szUnderscore + 1);
    }

    if (const char* szBracket = sCleanName.FindLastSubString("<"))
    {
      sCleanName = WStringView(sCleanName.GetStartPointer(), szBracket);
    }

    sFullPath = nodeTemplate.m_sCategory.GetString();
    if (sFullPath.IsEmpty())
    {
      if (auto pAttr = pRtti->GetAttributeByType<WCategoryAttribute>())
      {
        sFullPath = pAttr->GetCategory();
      }
    }

    sFullPath.AppendPath(sCleanName);

    m_NodeCreationTemplatePaths[i] = sFullPath;
    templateNames[i] = WTranslate(sCleanName.GetData(tmp));

    pSearchMenu->AddItem(templateNames[i], sFullPath, QVariant::fromValue(i));
  }

  // add the recently used ones at the top
  {
    WInt32 iToAdd = 8;

    for (const WString& sRecent : WQtSearchableMenuRecentList::GetList(m_sRecentListName))
    {
      const WUInt32 uiIndex = m_NodeCreationTemplatePaths.IndexOf(sRecent);

      if (uiIndex == WInvalidIndex)
        continue;

      sFullPath.Set(" *** RECENT ***/", m_NodeCreationTemplatePaths[uiIndex].GetView());

      pSearchMenu->AddItem(templateNames[uiIndex], sFullPath, QVariant::fromValue(uiIndex));

      if (--iToAdd <= 0)
        break;
    }
  }

  pSearchMenu->Finalize(m_sContextMenuSearchText);

  menu.exec(screenPos);

  m_sContextMenuSearchText = pSearchMenu->GetSearchText();
}

WStatus WQtVisualGraphScene::RemoveNode(WQtVisualGraphNode* pNode)
{
  W_SUCCEED_OR_RETURN(m_pManager->CanRemove(pNode->GetObject()));

  WRemoveNodeCommand cmd;
  cmd.m_Object = pNode->GetObject()->GetGuid();

  WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  return history->AddCommand(cmd);
}

void WQtVisualGraphScene::RemoveSelectedNodesAction()
{
  WDeque<WQtVisualGraphNode*> selection;
  GetSelectedNodes(selection);

  if (selection.IsEmpty())
    return;

  WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Nodes");

  for (WQtVisualGraphNode* pNode : selection)
  {
    WStatus res = RemoveNode(pNode);

    if (res.Failed())
    {
      history->CancelTransaction();

      WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to remove node");
      return;
    }
  }

  history->FinishTransaction();
}

void WQtVisualGraphScene::AddCommentAroundSelectionAction()
{
  constexpr float fPadding = 20.0f;
  constexpr float fHeaderHeight = 26.0f; // must match WQtVisualGraphCommentNode::s_fHeaderHeight

  WVec2 vPos = m_vMousePos;
  WVec2 vSize(300.0f, 200.0f);
  bool bCustomSize = false;

  WDeque<WQtVisualGraphNode*> selection;
  GetSelectedNodes(selection);

  if (!selection.IsEmpty())
  {
    QRectF bounds;
    for (const WQtVisualGraphNode* pNode : selection)
      bounds = bounds.united(pNode->sceneBoundingRect());

    vPos.x = static_cast<float>(bounds.left()) - fPadding;
    vPos.y = static_cast<float>(bounds.top()) - fHeaderHeight - fPadding;
    vSize.x = static_cast<float>(bounds.width()) + fPadding * 2.0f;
    vSize.y = static_cast<float>(bounds.height()) + fHeaderHeight + fPadding * 2.0f;
    bCustomSize = true;
  }

  WCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Comment");

  WStatus res(W_SUCCESS);
  {
    WAddObjectCommand cmd;
    cmd.m_pType = WGetStaticRTTI<WVisualGraphComment>();
    cmd.m_NewObjectGuid = WUuid::MakeUuid();
    cmd.m_Index = -1;
    res = history->AddCommand(cmd);

    if (res.Succeeded())
    {
      WMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = vPos;
      res = history->AddCommand(move);
    }

    if (res.Succeeded() && bCustomSize)
    {
      WSetObjectPropertyCommand setCmd;
      setCmd.m_Object = cmd.m_NewObjectGuid;
      setCmd.m_sProperty = "Size";
      setCmd.m_NewValue = vSize;
      res = history->AddCommand(setCmd);
    }
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding comment node failed.");
}

void WQtVisualGraphScene::ConnectPinsAction(const WVisualGraphPin& sourcePin, const WVisualGraphPin& targetPin)
{
  WVisualGraphObjectManager::CanConnectResult connect;
  WStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), sourcePin, targetPin, connect);

  if (connect == WVisualGraphObjectManager::CanConnectResult::ConnectNever)
  {
    WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
    return;
  }

  WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Connect Pins");

  // disconnect everything from the source pin
  if (connect == WVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == WVisualGraphObjectManager::CanConnectResult::Connect1toN)
  {
    const WArrayPtr<const WVisualGraphConnection* const> connections = m_pManager->GetConnections(sourcePin);
    for (const WVisualGraphConnection* pConnection : connections)
    {
      res = WNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // disconnect everything from the target pin
  if (connect == WVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == WVisualGraphObjectManager::CanConnectResult::ConnectNto1)
  {
    const WArrayPtr<const WVisualGraphConnection* const> connections = m_pManager->GetConnections(targetPin);
    for (const WVisualGraphConnection* pConnection : connections)
    {
      res = WNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // connect the two pins
  {
    res = WNodeCommands::AddAndConnectCommand(history, m_pManager->GetConnectionType(), sourcePin, targetPin);
    if (res.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void WQtVisualGraphScene::DisconnectPinsAction(WQtVisualGraphConnection* pConnection)
{
  WStatus res = m_pManager->CanDisconnect(pConnection->GetConnection());
  if (res.Succeeded())
  {
    WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Disconnect Pins");

    res = WNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetConnection()->GetParent()->GetGuid());
    if (res.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
}

void WQtVisualGraphScene::DisconnectPinsAction(WQtVisualGraphPin* pPin)
{
  WCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  WStatus res = WStatus(W_SUCCESS);
  for (WQtVisualGraphConnection* pConnection : pPin->GetConnections())
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void WQtVisualGraphScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  WUInt32 uiTypeIndex = variant.value<WUInt32>();
  if (uiTypeIndex >= m_NodeCreationTemplates.GetCount())
    return;

  if (uiTypeIndex < m_NodeCreationTemplatePaths.GetCount())
  {
    WQtSearchableMenuRecentList::UseEntry(m_sRecentListName, m_NodeCreationTemplatePaths[uiTypeIndex]);
  }

  CreateNodeObject(m_NodeCreationTemplates[uiTypeIndex]);
}

void WQtVisualGraphScene::OnSelectionChanged()
{
  WCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  if (pHistory->IsInUndoRedo() || pHistory->IsInTransaction())
    return;

  m_Selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == WQtVisualGraphScene::Node)
    {
      WQtVisualGraphNode* pNode = static_cast<WQtVisualGraphNode*>(pItem);
      m_Selection.PushBack(pNode->GetObject());
    }
    else if (pItem->type() == WQtVisualGraphScene::Connection)
    {
      WQtVisualGraphConnection* pConnection = static_cast<WQtVisualGraphConnection*>(pItem);
      m_Selection.PushBack(pConnection->GetObject());
    }
  }

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;
    m_pManager->GetDocument()->GetSelectionManager()->SetSelection(m_Selection);
    m_bIgnoreSelectionChange = false;
  }
}

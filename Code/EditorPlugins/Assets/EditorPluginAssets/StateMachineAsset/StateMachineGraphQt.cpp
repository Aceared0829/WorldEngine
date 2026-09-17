#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <Foundation/Math/ColorScheme.h>

WQtStateMachinePin::WQtStateMachinePin() = default;

void WQtStateMachinePin::SetPin(const WVisualGraphPin& pin)
{
  WQtVisualGraphPin::SetPin(pin);

  constexpr int padding = 3;

  m_pLabel->setPlainText("     +     ");
  m_pLabel->setPos(0, 0);

  auto bounds = m_pLabel->boundingRect();
  m_PinCenter = bounds.center();

  QPainterPath p;
  p.addRect(bounds);
  setPath(p);

  if (pin.GetType() == WVisualGraphPin::Type::Input)
  {
    m_pLabel->setPlainText("");
  }
  else
  {
    m_pLabel->setToolTip("Add Transition");
  }
}

QRectF WQtStateMachinePin::GetPinRect() const
{
  auto rect = path().boundingRect();
  rect.translate(pos());
  return rect;
}

//////////////////////////////////////////////////////////////////////////

WQtStateMachineConnection::WQtStateMachineConnection()
{
  setFlag(QGraphicsItem::ItemIsSelectable);
}

//////////////////////////////////////////////////////////////////////////

WQtStateMachineNode::WQtStateMachineNode() = default;

void WQtStateMachineNode::InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject)
{
  WQtVisualGraphNode::InitNode(pManager, pObject);

  UpdateHeaderColor();
}

void WQtStateMachineNode::UpdateGeometry()
{
  prepareGeometryChange();

  auto labelRect = m_pTitleLabel->boundingRect();

  constexpr int padding = 5;
  const int headerWidth = labelRect.width();
  const int headerHeight = labelRect.height() + padding * 2;

  int h = headerHeight;
  int w = headerWidth;

  for (WQtVisualGraphPin* pQtPin : GetInputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = WMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
  }

  for (WQtVisualGraphPin* pQtPin : GetOutputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = WMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
    h += rectPin.height();
  }

  w += padding * 2;
  h += padding * 2;

  m_HeaderRect = QRectF(-padding, -padding, w, headerHeight);

  {
    QPainterPath p;
    p.addRoundedRect(-padding, -padding, w, h, padding, padding);
    setPath(p);
  }
}

void WQtStateMachineNode::UpdateState()
{
  UpdateHeaderColor();

  if (IsAnyState())
  {
    m_pTitleLabel->setPlainText("Any State");
  }
  else
  {
    WStringBuilder sName;

    auto& typeAccessor = GetObject()->GetTypeAccessor();

    WVariant name = typeAccessor.GetValue("Name");
    if (name.IsA<WString>() && name.Get<WString>().IsEmpty() == false)
    {
      sName = name.Get<WString>();
    }
    else
    {
      sName = typeAccessor.GetType()->GetTypeName();
    }

    if (IsInitialState())
    {
      sName.Append(" [Initial State]");
    }

    m_pTitleLabel->setPlainText(sName.GetData());
  }
}

void WQtStateMachineNode::ExtendContextMenu(QMenu& ref_menu)
{
  if (IsAnyState())
    return;

  QAction* pAction = new QAction("Set as Initial State", &ref_menu);
  pAction->setEnabled(IsInitialState() == false);
  pAction->connect(pAction, &QAction::triggered,
    [this]()
    {
      auto pScene = static_cast<WQtStateMachineAssetScene*>(scene());
      pScene->SetInitialState(this);
    });

  ref_menu.addAction(pAction);
}

bool WQtStateMachineNode::IsInitialState() const
{
  auto pManager = static_cast<const WStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsInitialState(GetObject());
}

bool WQtStateMachineNode::IsAnyState() const
{
  auto pManager = static_cast<const WStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsAnyState(GetObject());
}

void WQtStateMachineNode::UpdateHeaderColor()
{
  WColorScheme::Enum schemeColor = WColorScheme::Gray;

  if (IsAnyState())
  {
    schemeColor = WColorScheme::Violet;
  }
  else if (IsInitialState())
  {
    schemeColor = WColorScheme::Teal;
  }

  m_HeaderColor = WToQtColor(WColorScheme::DarkUI(schemeColor));

  update();
}

//////////////////////////////////////////////////////////////////////////

WQtStateMachineAssetScene::WQtStateMachineAssetScene(QObject* pParent /*= nullptr*/)
  : WQtVisualGraphScene(pParent)
{
  SetConnectionStyle(WQtVisualGraphScene::ConnectionStyle::StraightLine);
  SetConnectionDecorationFlags(WQtVisualGraphScene::ConnectionDecorationFlags::DirectionArrows);
}

WQtStateMachineAssetScene::~WQtStateMachineAssetScene() = default;

void WQtStateMachineAssetScene::SetInitialState(WQtStateMachineNode* pNode)
{
  WCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Set Initial State");

  WStateMachine_SetInitialStateCommand cmd;
  cmd.m_NewInitialStateObject = pNode->GetObject()->GetGuid();

  WStatus res = history->AddCommand(cmd);

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

WStatus WQtStateMachineAssetScene::RemoveNode(WQtVisualGraphNode* pNode)
{
  auto pManager = static_cast<const WStateMachineNodeManager*>(GetDocumentNodeManager());
  const bool bWasInitialState = pManager->IsInitialState(pNode->GetObject());

  auto res = WQtVisualGraphScene::RemoveNode(pNode);
  if (res.Succeeded() && bWasInitialState)
  {
    // Find another node
    WUuid newInitialStateObject;
    for (auto it : m_Nodes)
    {
      if (it.Value() != pNode && pManager->IsAnyState(it.Key()) == false)
      {
        newInitialStateObject = it.Key()->GetGuid();
      }
    }

    if (newInitialStateObject.IsValid())
    {
      WCommandHistory* history = pManager->GetDocument()->GetCommandHistory();

      WStateMachine_SetInitialStateCommand cmd;
      cmd.m_NewInitialStateObject = newInitialStateObject;

      res = history->AddCommand(cmd);
    }
  }

  return res;
}

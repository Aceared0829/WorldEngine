#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/CommentNode.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>

#include <QApplication>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

WQtVisualGraphCommentNode::WQtVisualGraphCommentNode()
{
  m_CommentColor = QColor(70, 70, 70, 200);

  setZValue(-100); // render below all regular nodes
  setAcceptHoverEvents(true);
}

void WQtVisualGraphCommentNode::InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject)
{
  m_pManager = pManager;

  m_pCommentLabel = new QGraphicsTextItem(this);
  m_pCommentLabel->setAcceptHoverEvents(false); // let hover events fall through to the parent so resize cursor detection works correctly

  QFont font = QApplication::font();
  font.setPointSizeF(font.pointSizeF() * 1.1f);
  m_pCommentLabel->setFont(font);
  m_pCommentLabel->setDefaultTextColor(QColor(220, 220, 220));
  m_pCommentLabel->setTextWidth(-1); // single line, no wrap

  // Call base to hook up document object and flags
  WQtVisualGraphNode::InitNode(pManager, pObject);

  // Hide base class title/subtitle/icon since we render our own text
  m_pTitleLabel->setVisible(false);
  m_pSubtitleLabel->setVisible(false);
  m_pIcon->setVisible(false);
}

void WQtVisualGraphCommentNode::UpdateGeometry()
{
  prepareGeometryChange();

  const WDocumentObject* pObject = GetObject();
  if (pObject)
  {
    WVariant sizeVar = pObject->GetTypeAccessor().GetValue("Size");
    if (sizeVar.IsA<WVec2>())
    {
      m_vCurrentSize = sizeVar.Get<WVec2>();
    }
  }

  m_vCurrentSize.x = WMath::Max(m_vCurrentSize.x, s_fMinWidth);
  m_vCurrentSize.y = WMath::Max(m_vCurrentSize.y, s_fMinHeight);

  const float padding = 8.0f;

  QPainterPath p;
  p.addRoundedRect(0, 0, m_vCurrentSize.x, m_vCurrentSize.y, 8, 8);
  setPath(p);

  m_pCommentLabel->setPos(padding, (s_fHeaderHeight - m_pCommentLabel->boundingRect().height()) * 0.5f);
  m_pCommentLabel->setTextWidth(m_vCurrentSize.x - padding * 2);
}

void WQtVisualGraphCommentNode::UpdateState()
{
  const WDocumentObject* pObject = GetObject();
  if (!pObject)
    return;

  WVariant commentVar = pObject->GetTypeAccessor().GetValue("Comment");
  if (commentVar.IsA<WString>())
  {
    m_pCommentLabel->setPlainText(commentVar.Get<WString>().GetData());
  }

  WVariant colorVar = pObject->GetTypeAccessor().GetValue("Color");
  if (colorVar.IsA<WColorGammaUB>())
  {
    m_CommentColor = WToQtColor(colorVar.Get<WColorGammaUB>());
  }
}

void WQtVisualGraphCommentNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  if (GetFlags().IsSet(WQtVisualGraphNodeFlags::UpdateTitle))
  {
    UpdateState();
    // Only rebuild geometry from document if we're not actively resizing
    if (m_uiActiveResizeEdge == None)
    {
      UpdateGeometry();
    }
  }

  auto palette = QApplication::palette();

  // Background
  QColor backgroundColor = m_CommentColor;
  backgroundColor.setAlphaF(0.3f);

  painter->setPen(Qt::NoPen);
  painter->setBrush(backgroundColor);
  painter->drawPath(path());

  // Header bar
  {
    QColor headerColor = m_CommentColor.darker(130);
    const QRectF bounds = path().boundingRect();

    painter->setClipPath(path());
    painter->setBrush(headerColor);
    painter->drawRect(bounds.x(), bounds.y(), bounds.width(), s_fHeaderHeight);
    painter->setClipping(false);
  }

  // Outline
  if (isSelected())
  {
    QPen p(palette.highlight().color(), 2, Qt::SolidLine);
    painter->setPen(p);
  }
  else
  {
    QPen p(m_CommentColor.darker(150), 1, Qt::SolidLine);
    painter->setPen(p);
  }

  painter->setBrush(Qt::NoBrush);
  painter->drawPath(path());

  // Invert the theme text color when the comment background is light,
  // so that the label stays readable on both dark and light comment colors.
  QColor textColor = palette.buttonText().color();
  const bool bBackgroundIsLight = m_CommentColor.lightnessF() > 0.6f;
  if (bBackgroundIsLight)
  {
    textColor.setRed(255 - textColor.red());
    textColor.setGreen(255 - textColor.green());
    textColor.setBlue(255 - textColor.blue());
  }

  m_pCommentLabel->setDefaultTextColor(textColor);
}

WUInt8 WQtVisualGraphCommentNode::DetectResizeEdge(const QPointF& localPos) const
{
  const QRectF bounds = path().boundingRect();
  WUInt8 uiEdge = None;

  if (localPos.x() < bounds.left() + s_fEdgeThreshold)
    uiEdge |= Left;
  else if (localPos.x() > bounds.right() - s_fEdgeThreshold)
    uiEdge |= Right;

  if (localPos.y() < bounds.top() + s_fEdgeThreshold)
    uiEdge |= Top;
  else if (localPos.y() > bounds.bottom() - s_fEdgeThreshold)
    uiEdge |= Bottom;

  return uiEdge;
}

void WQtVisualGraphCommentNode::UpdateCursorForEdge(WUInt8 uiEdge)
{
  if (uiEdge == (Left | Top) || uiEdge == (Right | Bottom))
    setCursor(Qt::SizeFDiagCursor);
  else if (uiEdge == (Right | Top) || uiEdge == (Left | Bottom))
    setCursor(Qt::SizeBDiagCursor);
  else if (uiEdge & (Left | Right))
    setCursor(Qt::SizeHorCursor);
  else if (uiEdge & (Top | Bottom))
    setCursor(Qt::SizeVerCursor);
  else
    unsetCursor();
}

void WQtVisualGraphCommentNode::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
  UpdateCursorForEdge(DetectResizeEdge(event->pos()));
  QGraphicsPathItem::hoverMoveEvent(event);
}

void WQtVisualGraphCommentNode::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    WQtVisualGraphNode::mousePressEvent(event);
    return;
  }

  WUInt8 uiEdge = DetectResizeEdge(event->pos());

  if (uiEdge != None)
  {
    // Start resize: disable movement, track resize state
    m_uiActiveResizeEdge = uiEdge;
    m_ResizeStartMouseScene = event->scenePos();
    m_ResizeStartPos = pos();
    m_vResizeStartSize = m_vCurrentSize;

    setFlag(QGraphicsItem::ItemIsMovable, false);
    event->accept();
    return;
  }

  // Normal move: find nodes contained within this comment
  m_ContainedNodes.Clear();
  m_PrevPos = pos();

  if (scene() && !(event->modifiers() & Qt::Modifier::SHIFT)) // holding SHIFT deactivates movement of contained nodes
  {
    QRectF sceneRect = mapToScene(path().boundingRect()).boundingRect();
    QList<QGraphicsItem*> items = scene()->items(sceneRect, Qt::IntersectsItemBoundingRect);

    for (QGraphicsItem* pItem : items)
    {
      if (pItem == this || pItem->parentItem() != nullptr) // skip self and child items (e.g. pin graphics items)
        continue;

      // Only include top-level items that are our node type
      if (pItem->type() != WQtVisualGraphScene::Node)
        continue;

      // Only if the node's center is inside the comment bounds
      // Skip selected nodes: they are already moved by the scene's selection logic,
      // so adding them here would apply the movement delta twice.
      QPointF nodeCenter = pItem->mapToScene(pItem->boundingRect().center());
      if (sceneRect.contains(nodeCenter) && !pItem->isSelected())
      {
        m_ContainedNodes.PushBack(pItem);
      }
    }
  }

  WQtVisualGraphNode::mousePressEvent(event);
}

void WQtVisualGraphCommentNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_uiActiveResizeEdge != None)
  {
    QPointF delta = event->scenePos() - m_ResizeStartMouseScene;

    WVec2 newSize = m_vResizeStartSize;
    QPointF newPos = m_ResizeStartPos;

    if (m_uiActiveResizeEdge & Right)
      newSize.x = WMath::Max(m_vResizeStartSize.x + (float)delta.x(), s_fMinWidth);

    if (m_uiActiveResizeEdge & Bottom)
      newSize.y = WMath::Max(m_vResizeStartSize.y + (float)delta.y(), s_fMinHeight);

    if (m_uiActiveResizeEdge & Left)
    {
      float maxDeltaX = m_vResizeStartSize.x - s_fMinWidth;
      float dx = WMath::Clamp((float)delta.x(), -10000.0f, maxDeltaX);
      newSize.x = m_vResizeStartSize.x - dx;
      newPos.setX(m_ResizeStartPos.x() + dx);
    }

    if (m_uiActiveResizeEdge & Top)
    {
      float maxDeltaY = m_vResizeStartSize.y - s_fMinHeight;
      float dy = WMath::Clamp((float)delta.y(), -10000.0f, maxDeltaY);
      newSize.y = m_vResizeStartSize.y - dy;
      newPos.setY(m_ResizeStartPos.y() + dy);
    }

    m_vCurrentSize = newSize;

    // Update visual path directly (no document commit until release)
    prepareGeometryChange();
    QPainterPath p;
    p.addRoundedRect(0, 0, m_vCurrentSize.x, m_vCurrentSize.y, 8, 8);
    setPath(p);

    float padding = 8.0f;
    m_pCommentLabel->setTextWidth(m_vCurrentSize.x - padding * 2);

    setPos(newPos);

    event->accept();
    return;
  }

  WQtVisualGraphNode::mouseMoveEvent(event);
}

void WQtVisualGraphCommentNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_uiActiveResizeEdge != None && event->button() == Qt::LeftButton)
  {
    m_uiActiveResizeEdge = None;
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // Commit size and position changes to the document
    if (m_pManager && GetObject())
    {
      WCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
      pHistory->StartTransaction("Resize Comment");

      WStatus res(W_SUCCESS);

      // Commit new size
      {
        WSetObjectPropertyCommand cmd;
        cmd.m_Object = GetObject()->GetGuid();
        cmd.m_sProperty = "Size";
        cmd.m_NewValue = m_vCurrentSize;
        res = pHistory->AddCommand(cmd);
      }

      // Commit new position if it changed (left/top edge resize)
      if (res.Succeeded())
      {
        WMoveNodeCommand move;
        move.m_Object = GetObject()->GetGuid();
        move.m_NewPos = WVec2((float)pos().x(), (float)pos().y());
        res = pHistory->AddCommand(move);
      }

      if (res.Failed())
        pHistory->CancelTransaction();
      else
        pHistory->FinishTransaction();
    }

    event->accept();
    return;
  }

  // Clear contained nodes after the base class and scene process the move
  m_ContainedNodes.Clear();

  WQtVisualGraphNode::mouseReleaseEvent(event);
}

QVariant WQtVisualGraphCommentNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == QGraphicsItem::ItemPositionHasChanged && !m_ContainedNodes.IsEmpty())
  {
    // Only move contained nodes during interactive dragging, not during undo/redo
    if (m_pManager)
    {
      WCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
      {
        QPointF newPos = value.toPointF();
        QPointF delta = newPos - m_PrevPos;
        m_PrevPos = newPos;

        for (QGraphicsItem* pItem : m_ContainedNodes)
        {
          pItem->moveBy(delta.x(), delta.y());
        }
      }
    }
  }

  return WQtVisualGraphNode::itemChange(change, value);
}

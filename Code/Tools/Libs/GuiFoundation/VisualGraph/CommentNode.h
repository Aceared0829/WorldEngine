#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Node.h>

class QGraphicsTextItem;
class WVisualGraphObjectManager;

/// Qt graphics item for comment boxes in visual graphs, similar to Blueprint comments.
class W_GUIFOUNDATION_DLL WQtVisualGraphCommentNode : public WQtVisualGraphNode
{
public:
  WQtVisualGraphCommentNode();

  virtual void InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
  enum ResizeEdge : WUInt8
  {
    None = 0,
    Left = W_BIT(0),
    Right = W_BIT(1),
    Top = W_BIT(2),
    Bottom = W_BIT(3),
  };

  WUInt8 DetectResizeEdge(const QPointF& localPos) const;
  void UpdateCursorForEdge(WUInt8 uiEdge);

  const WVisualGraphObjectManager* m_pManager = nullptr;
  QGraphicsTextItem* m_pCommentLabel = nullptr;
  QColor m_CommentColor;

  // Resize state
  WUInt8 m_uiActiveResizeEdge = None;
  QPointF m_ResizeStartMouseScene;
  QPointF m_ResizeStartPos;
  WVec2 m_vResizeStartSize;
  WVec2 m_vCurrentSize = WVec2(300, 200);

  // Containment tracking: when moving the comment, contained nodes move with it
  QPointF m_PrevPos;
  WDynamicArray<QGraphicsItem*> m_ContainedNodes;

  static constexpr float s_fEdgeThreshold = 10.0f;
  static constexpr float s_fHeaderHeight = 26.0f;
  static constexpr float s_fMinWidth = 120.0f;
  static constexpr float s_fMinHeight = 80.0f;
};

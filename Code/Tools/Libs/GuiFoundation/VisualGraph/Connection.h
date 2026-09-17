#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsPathItem>

class WVisualGraphPin;

/// Qt graphics item representing a connection between two pins in a visual graph.
///
/// Renders the visual connection path between pins using different styles (bezier curves, straight lines, or subway-style routing).
/// Updates its geometry automatically when connected pins move.
class W_GUIFOUNDATION_DLL WQtVisualGraphConnection : public QGraphicsPathItem
{
public:
  explicit WQtVisualGraphConnection(QGraphicsItem* pParent = 0);
  ~WQtVisualGraphConnection();
  virtual int type() const override { return WQtVisualGraphScene::Connection; }

  const WDocumentObject* GetObject() const { return m_pObject; }
  const WVisualGraphConnection* GetConnection() const { return m_pConnection; }
  void InitConnection(const WDocumentObject* pObject, const WVisualGraphConnection* pConnection);

  void SetPosIn(const QPointF& point);
  void SetPosOut(const QPointF& point);
  void SetDirIn(const QPointF& dir);
  void SetDirOut(const QPointF& dir);

  virtual void UpdateGeometry();
  virtual QPen DeterminePen() const;

  const QPointF& GetInPos() const { return m_InPoint; }
  const QPointF& GetOutPos() const { return m_OutPoint; }

  bool m_bAdjacentNodeSelected = false;
  /// When set, the connection is drawn thicker and brighter to indicate it is carrying a live/active signal (e.g. for debug visualization).
  bool m_bHighlight = false;

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // Draws connections following the rules of subway maps (angles of 45 degrees only).
  void DrawSubwayPath(QPainterPath& path, const QPointF& startPoint, const QPointF& endPoint);

  const WDocumentObject* m_pObject = nullptr;
  const WVisualGraphConnection* m_pConnection = nullptr;

  QPointF m_InPoint;
  QPointF m_OutPoint;
  QPointF m_InDir;
  QPointF m_OutDir;
};

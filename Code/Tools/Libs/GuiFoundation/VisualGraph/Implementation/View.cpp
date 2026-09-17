#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

WQtVisualGraphView::WQtVisualGraphView(QWidget* pParent)
  : QGraphicsView(pParent)

{
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_ViewPos = QPointF(0, 0);
  m_ViewScale = QPointF(1, 1);
}

WQtVisualGraphView::~WQtVisualGraphView() = default;

void WQtVisualGraphView::SetScene(WQtVisualGraphScene* pScene)
{
  m_pScene = pScene;
  setScene(pScene);

  QRectF sceneRect = m_pScene->sceneRect();
  m_ViewPos = sceneRect.topLeft();
  m_ViewScale = QPointF(1, 1);
  m_bFrameOnNextDraw = true;
  UpdateView();
}

WQtVisualGraphScene* WQtVisualGraphView::GetScene()
{
  return m_pScene;
}

void WQtVisualGraphView::FrameContent()
{
  if (m_pScene == nullptr || width() == 0 || height() == 0)
    return;

  // use selected nodes if any, otherwise all visible top-level items
  QRectF contentRect;
  const QList<QGraphicsItem*> selection = m_pScene->selectedItems();
  if (!selection.isEmpty())
  {
    for (QGraphicsItem* pItem : selection)
    {
      if (pItem->isVisible())
        contentRect = contentRect.united(pItem->sceneBoundingRect());
    }
  }
  else
  {
    for (QGraphicsItem* pItem : m_pScene->items())
    {
      if (pItem->isVisible() && pItem->type() == WQtVisualGraphScene::Node)
        contentRect = contentRect.united(pItem->sceneBoundingRect());
    }
  }

  if (contentRect.isEmpty())
    return;

  // margin
  const double fMargin = 50.0;
  contentRect.adjust(-fMargin, -fMargin, fMargin, fMargin);

  const double fScaleX = width() / contentRect.width();
  const double fScaleY = height() / contentRect.height();
  double fScale = qMin(fScaleX, fScaleY);
  fScale = WMath::Clamp(fScale, 0.01, 2.0);

  m_ViewScale = QPointF(fScale, fScale);

  const double fViewWidth = width() / fScale;
  const double fViewHeight = height() / fScale;
  m_ViewPos = QPointF(contentRect.center().x() - fViewWidth * 0.5, contentRect.center().y() - fViewHeight * 0.5);

  UpdateView();
}

void WQtVisualGraphView::mousePressEvent(QMouseEvent* event)
{
  QGraphicsView::mousePressEvent(event);

  if (event->button() == Qt::RightButton)
  {
    setContextMenuPolicy(Qt::NoContextMenu);
    m_StartDragView = event->position();

    m_StartDragScene = m_ViewPos;
    viewport()->setCursor(Qt::ClosedHandCursor);
    event->accept();
    m_bPanning = true;
    m_iPanCounter = 0;
  }
}

void WQtVisualGraphView::mouseMoveEvent(QMouseEvent* event)
{
  QGraphicsView::mouseMoveEvent(event);

  if (m_bPanning)
  {
    m_iPanCounter++;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QPointF vViewDelta = m_StartDragView - event->position();
#else
    QPointF vViewDelta = m_vStartDragView - event->pos();
#endif
    QPointF vSceneDelta = QPointF(vViewDelta.x() / m_ViewScale.x(), vViewDelta.y() / m_ViewScale.y());
    m_ViewPos = m_StartDragScene + vSceneDelta;
    UpdateView();
  }
}

void WQtVisualGraphView::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_bPanning)
  {
    viewport()->setCursor(Qt::ArrowCursor);
    event->accept();
    m_bPanning = false;
    setContextMenuPolicy(Qt::DefaultContextMenu);
    if (m_iPanCounter > 2)
      return;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void WQtVisualGraphView::wheelEvent(QWheelEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF centerA(event->position().x() / m_ViewScale.x(), event->position().y() / m_ViewScale.y());
#else
  QPointF centerA(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());
#endif

  const qreal fScaleFactor = 1.15;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  const qreal fScale = (event->angleDelta().y() > 0) ? fScaleFactor : (1.0 / fScaleFactor);
#else
  const qreal fScale = (event->delta() > 0) ? fScaleFactor : (1.0 / fScaleFactor);
#endif

  m_ViewScale *= fScale;
  m_ViewScale.setX(WMath::Clamp(m_ViewScale.x(), 0.01, 2.0));
  m_ViewScale.setY(WMath::Clamp(m_ViewScale.y(), 0.01, 2.0));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF centerB(event->position().x() / m_ViewScale.x(), event->position().y() / m_ViewScale.y());
#else
  QPointF centerB(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());
#endif
  m_ViewPos -= (centerB - centerA);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  m_StartDragView = event->position();
#else
  m_vStartDragView = event->pos();
#endif
  m_StartDragScene = m_ViewPos;

  UpdateView();
}

void WQtVisualGraphView::contextMenuEvent(QContextMenuEvent* event)
{
  if (m_iPanCounter > 2)
  {
    m_iPanCounter = 0;
    return;
  }
  QGraphicsView::contextMenuEvent(event);
}

void WQtVisualGraphView::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_F && event->modifiers() == Qt::NoModifier)
  {
    FrameContent();
    event->accept();
    return;
  }

  QGraphicsView::keyPressEvent(event);
}

void WQtVisualGraphView::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);

  UpdateView();
}

void WQtVisualGraphView::drawBackground(QPainter* painter, const QRectF& r)
{
  if (m_bFrameOnNextDraw)
  {
    // Frame is deferred until drawBackground because SetScene() is called before the
    // widget has its final geometry, so width()/height() would be zero at that point.
    m_bFrameOnNextDraw = false;
    QTimer::singleShot(10, this, &WQtVisualGraphView::FrameContent);
  }

  QGraphicsView::drawBackground(painter, r);

  if (m_ViewScale.manhattanLength() > 1.0)
  {
    QPen pfine(WToQtColor(WColorScheme::GetColor(WColorScheme::Gray, 0)), 1.0);

    painter->setPen(pfine);
    DrawGrid(painter, 15);
  }

  if (m_ViewScale.manhattanLength() > 0.1)
  {
    double scale = m_ViewScale.manhattanLength() < 0.25 ? 150.0 : 300.0;

    QPen p(WToQtColor(WColorScheme::GetColor(WColorScheme::Gray, 1)), 1.0);

    painter->setPen(p);
    DrawGrid(painter, scale);
  }

  // Only force constant redraws when doing the debug animation.
  if (GetScene()->GetConnectionDecorationFlags().IsSet(WQtVisualGraphScene::ConnectionDecorationFlags::DrawDebugging))
  {
    UpdateView();
  }
}

void WQtVisualGraphView::UpdateView()
{
  QRectF sceneRect(m_ViewPos.x(), m_ViewPos.y(), width() / m_ViewScale.x(), height() / m_ViewScale.y());
  setSceneRect(sceneRect);
  fitInView(sceneRect, Qt::KeepAspectRatio);
}

void WQtVisualGraphView::DrawGrid(QPainter* painter, const double gridStep)
{
  const QRectF sceneRect(m_ViewPos.x(), m_ViewPos.y(), width() / m_ViewScale.x(), height() / m_ViewScale.y());
  const QPointF topLeft = sceneRect.topLeft();
  const QPointF bottomRight = sceneRect.bottomRight();

  const double left = WMath::Floor(topLeft.x() / gridStep - 0.5);
  const double right = WMath::Floor(bottomRight.x() / gridStep + 1.0);
  const double bottom = WMath::Floor(topLeft.y() / gridStep - 0.5);
  const double top = WMath::Floor(bottomRight.y() / gridStep + 1.0);

  // vertical lines
  for (int xi = static_cast<int>(left); xi <= static_cast<int>(right); ++xi)
  {
    QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep, top * gridStep);
    painter->drawLine(line);
  }

  // horizontal lines
  for (int yi = static_cast<int>(bottom); yi <= static_cast<int>(top); ++yi)
  {
    QLineF line(left * gridStep, yi * gridStep, right * gridStep, yi * gridStep);
    painter->drawLine(line);
  }
}

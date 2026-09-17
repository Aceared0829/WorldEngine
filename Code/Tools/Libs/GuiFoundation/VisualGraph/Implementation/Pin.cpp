#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <QApplication>
#include <QPalette>

WQtVisualGraphPin::WQtVisualGraphPin()
{
  auto palette = QApplication::palette();

  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(palette.base());

  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

  m_pLabel = new QGraphicsTextItem(this);
}

WQtVisualGraphPin::~WQtVisualGraphPin() = default;

void WQtVisualGraphPin::AddConnection(WQtVisualGraphConnection* pConnection)
{
  W_ASSERT_DEBUG(!m_Connections.Contains(pConnection), "Connection already present!");
  m_Connections.PushBack(pConnection);

  ConnectedStateChanged(true);

  UpdateConnections();
}

void WQtVisualGraphPin::RemoveConnection(WQtVisualGraphConnection* pConnection)
{
  W_ASSERT_DEBUG(m_Connections.Contains(pConnection), "Connection not present!");
  m_Connections.RemoveAndSwap(pConnection);

  if (m_Connections.IsEmpty())
    ConnectedStateChanged(false);
}

void WQtVisualGraphPin::ConnectedStateChanged(bool bConnected)
{
  UpdatePinColors();
}

void WQtVisualGraphPin::SetPin(const WVisualGraphPin& pin)
{
  m_pPin = &pin;

  if (m_bTranslatePinName)
  {
    m_pLabel->setPlainText(WMakeQString(WTranslate(pin.GetName())));
  }
  else
  {
    m_pLabel->setPlainText(pin.GetName());
  }

  auto rectLabel = m_pLabel->boundingRect();

  const int iRadus = rectLabel.height();
  QRectF bounds;

  if (pin.GetType() == WVisualGraphPin::Type::Input)
  {
    m_pLabel->setPos(iRadus, 0);
    bounds = QRectF(0, 0, iRadus, iRadus);
  }
  else
  {
    m_pLabel->setPos(0, 0);
    bounds = QRectF(rectLabel.width(), 0, iRadus, iRadus);
  }

  const int shrink = 3;
  bounds.adjust(shrink, shrink, -shrink, -shrink);
  m_PinCenter = bounds.center();

  {
    QPainterPath p;
    switch (m_pPin->m_Shape)
    {
      case WVisualGraphPin::Shape::Circle:
        p.addEllipse(bounds);
        break;
      case WVisualGraphPin::Shape::Rect:
        p.addRect(bounds);
        break;
      case WVisualGraphPin::Shape::RoundRect:
        p.addRoundedRect(bounds, 2, 2);
        break;
      case WVisualGraphPin::Shape::Arrow:
      {
        QPolygonF arrow;
        arrow.append(bounds.topLeft());
        arrow.append(QPointF(bounds.center().x(), bounds.top()));
        arrow.append(QPointF(bounds.right(), bounds.center().y()));
        arrow.append(QPointF(bounds.center().x(), bounds.bottom()));
        arrow.append(bounds.bottomLeft());
        arrow.append(bounds.topLeft());

        p.addPolygon(arrow);
        break;
      }
        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    setPath(p);
  }

  UpdatePinColors();
}

QPointF WQtVisualGraphPin::GetPinPos() const
{
  return mapToScene(m_PinCenter);
}

QPointF WQtVisualGraphPin::GetPinDir() const
{
  if (m_pPin->GetType() == WVisualGraphPin::Type::Input)
  {
    return QPointF(-1.0f, 0.0f);
  }
  else
  {
    return QPointF(1.0f, 0.0f);
    ;
  }
}

QRectF WQtVisualGraphPin::GetPinRect() const
{
  auto rectLabel = m_pLabel->boundingRect();
  rectLabel.translate(m_pLabel->pos());

  if (m_pPin->GetType() == WVisualGraphPin::Type::Input)
  {
    rectLabel.adjust(-5, 0, 0, 0);
  }
  else
  {
    rectLabel.adjust(0, 0, 5, 0);
  }
  return rectLabel;
}

void WQtVisualGraphPin::UpdateConnections()
{
  for (WQtVisualGraphConnection* pConnection : m_Connections)
  {
    if (m_pPin->GetType() == WVisualGraphPin::Type::Input)
    {
      pConnection->SetDirIn(GetPinDir());
      pConnection->SetPosIn(GetPinPos());
    }
    else
    {
      pConnection->SetDirOut(GetPinDir());
      pConnection->SetPosOut(GetPinPos());
    }
  }
}

void WQtVisualGraphPin::SetHighlightState(WQtVisualGraphPinHighlight state)
{
  if (m_HighlightState != state)
  {
    m_HighlightState = state;

    if (UpdatePinColors())
    {
      update();
    }
  }
}

void WQtVisualGraphPin::SetActive(bool bActive)
{
  m_bIsActive = bActive;

  if (UpdatePinColors())
  {
    update();
  }
}

bool WQtVisualGraphPin::UpdatePinColors(const WColorGammaUB* pOverwriteColor)
{
  WColorGammaUB pinColor = pOverwriteColor != nullptr ? *pOverwriteColor : GetPin()->GetColor();
  QColor base = QApplication::palette().window().color();

  if (!m_bIsActive)
    pinColor = WMath::Lerp<WColor>(WColorGammaUB(base.red(), base.green(), base.blue()), pinColor, 0.2f);

  switch (m_HighlightState)
  {
    case WQtVisualGraphPinHighlight::None:
    {
      QPen p = pen();
      p.setColor(WToQtColor(pinColor));
      setPen(p);

      setBrush(HasAnyConnections() ? pen().color().darker(125) : base);
    }
    break;

    case WQtVisualGraphPinHighlight::CannotConnect:
    case WQtVisualGraphPinHighlight::CannotConnectSameDirection:
    {
      QPen p = pen();
      p.setColor(base.lighter());
      setPen(p);

      setBrush(base);
    }
    break;

    case WQtVisualGraphPinHighlight::CanReplaceConnection:
    case WQtVisualGraphPinHighlight::CanAddConnection:
    {
      QPen p = pen();
      p.setColor(WToQtColor(pinColor));
      setPen(p);

      setBrush(base);
    }
    break;
  }

  QColor labelColor = QApplication::palette().buttonText().color();
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  m_pLabel->setDefaultTextColor(labelColor);

  return true;
}

QVariant WQtVisualGraphPin::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == QGraphicsItem::ItemScenePositionHasChanged)
  {
    UpdateConnections();
  }

  return QGraphicsPathItem::itemChange(change, value);
}

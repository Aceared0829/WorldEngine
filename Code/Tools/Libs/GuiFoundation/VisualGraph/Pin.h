#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsPathItem>

class WVisualGraphPin;
class WQtVisualGraphConnection;

/// Visual feedback state for pins during connection dragging
enum class WQtVisualGraphPinHighlight
{
  None,
  CannotConnect,
  CannotConnectSameDirection,
  CanAddConnection,
  CanReplaceConnection,
};

/// Qt graphics item representing a pin (connection point) on a visual graph node.
///
/// Displays the pin shape, color, and label. Manages visual feedback during connection operations
/// and maintains references to all connections attached to this pin.
class W_GUIFOUNDATION_DLL WQtVisualGraphPin : public QGraphicsPathItem
{
public:
  WQtVisualGraphPin();
  ~WQtVisualGraphPin();
  virtual int type() const override { return WQtVisualGraphScene::Pin; }

  void AddConnection(WQtVisualGraphConnection* pConnection);
  void RemoveConnection(WQtVisualGraphConnection* pConnection);
  WArrayPtr<WQtVisualGraphConnection*> GetConnections() { return m_Connections; }
  bool HasAnyConnections() const { return !m_Connections.IsEmpty(); }

  const WVisualGraphPin* GetPin() const { return m_pPin; }
  virtual void SetPin(const WVisualGraphPin& pin);
  virtual void ConnectedStateChanged(bool bConnected);

  virtual QPointF GetPinPos() const;
  virtual QPointF GetPinDir() const;
  virtual QRectF GetPinRect() const;
  virtual void UpdateConnections();
  void SetHighlightState(WQtVisualGraphPinHighlight state);

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}
  virtual void keyPressEvent(QKeyEvent* pEvent) override {}

protected:
  virtual bool UpdatePinColors(const WColorGammaUB* pOverwriteColor = nullptr);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  WQtVisualGraphPinHighlight m_HighlightState = WQtVisualGraphPinHighlight::None;
  QGraphicsTextItem* m_pLabel;
  QPointF m_PinCenter;

  bool m_bTranslatePinName = true;

private:
  bool m_bIsActive = true;

  const WVisualGraphPin* m_pPin = nullptr;
  WHybridArray<WQtVisualGraphConnection*, 6> m_Connections;
};

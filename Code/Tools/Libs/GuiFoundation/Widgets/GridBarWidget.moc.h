#pragma once

#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QPaintEvent;

class W_GUIFOUNDATION_DLL WQGridBarWidget : public QWidget
{
  Q_OBJECT

public:
  WQGridBarWidget(QWidget* pParent);

  void SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, WDelegate<QPointF(const QPointF&)> mapFromSceneFunc);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  QRectF m_ViewportSceneRect;
  double m_fTextGridStops;
  double m_fFineGridStops;
  WDelegate<QPointF(const QPointF&)> m_MapFromSceneFunc;
};

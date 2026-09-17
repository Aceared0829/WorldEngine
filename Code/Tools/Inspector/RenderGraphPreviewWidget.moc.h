#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>
#include <QImage>
#include <QPoint>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

/// Handles viewport interaction for the render graph texture preview.
class WQtRenderGraphPreviewWidget : public QWidget
{
  Q_OBJECT

public:
  explicit WQtRenderGraphPreviewWidget(QWidget* pParent = nullptr);

  void SetTextureSize(WVec2U32 vSize);
  void SetTargetSize(WVec2U32 vSize);
  void SetView(float fZoom, WVec2 vPanCenter);

Q_SIGNALS:
  void RequestChanged(float fZoom, WVec2 vPanCenter, WVec2I32 vPixel, bool bUpdatePixelPosition, bool bHighlightPixel);

protected:
  void paintEvent(QPaintEvent*) override;
  void wheelEvent(QWheelEvent* e) override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;

private:
  WVec2 GetUvExtents() const;
  WVec2 GetUvAtWidgetPosition(const QPoint& pos) const;
  QRect GetPreviewRect() const;
  void EmitChange(const QPoint& pos);

  WVec2U32 m_vTextureSize = WVec2U32(0, 0);
  WVec2U32 m_vTargetSize = WVec2U32(0, 0);
  float m_fZoom = 1.0f;
  WVec2 m_vPanCenter = WVec2(0.5f);
  bool m_bDragging = false;
  bool m_bPixelSelectionActive = false;
  QPoint m_LastMousePos;
  QImage m_CheckerboardImage;
};

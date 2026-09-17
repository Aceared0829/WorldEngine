#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ImageWidget.h>
#include <QGraphicsScene>

class QGraphicsPixmapItem;

class W_GUIFOUNDATION_DLL WQtImageScene : public QGraphicsScene
{
public:
  WQtImageScene(QObject* pParent = nullptr);

  void SetImage(QPixmap pixmap);

private:
  QPixmap m_Pixmap;
  QGraphicsPixmapItem* m_pImageItem;
};

class W_GUIFOUNDATION_DLL WQtImageWidget : public QWidget, public Ui_ImageWidget
{
  Q_OBJECT

public:
  WQtImageWidget(QWidget* pParent, bool bShowButtons = true);
  ~WQtImageWidget();

  void SetImage(QPixmap pixmap);

  void SetImageSize(float fScale = 1.0f);
  void ScaleImage(float fFactor);

private Q_SLOTS:

  void on_ButtonZoomIn_clicked();
  void on_ButtonZoomOut_clicked();
  void on_ButtonResetZoom_clicked();

private:
  void ImageApplyScale();

  WQtImageScene* m_pScene;
  float m_fCurrentScale;
};

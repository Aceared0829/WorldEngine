#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class W_GUIFOUNDATION_DLL WQtColorAreaWidget : public QWidget
{
  Q_OBJECT
public:
  WQtColorAreaWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

  float GetSaturation() const { return m_fSaturation; }
  void SetSaturation(float fSat);

  float GetValue() const { return m_fValue; }
  void SetValue(float fVal);

Q_SIGNALS:
  void valueChanged(double x, double y);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
  float m_fSaturation;
  float m_fValue;
};

class W_GUIFOUNDATION_DLL WQtColorRangeWidget : public QWidget
{
  Q_OBJECT
public:
  WQtColorRangeWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

Q_SIGNALS:
  void valueChanged(double x);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
};

class W_GUIFOUNDATION_DLL WQtColorCompareWidget : public QWidget
{
  Q_OBJECT
public:
  WQtColorCompareWidget(QWidget* pParent);

  void SetNewColor(const WColor& color);
  void SetInitialColor(const WColor& color);

protected:
  virtual void paintEvent(QPaintEvent*) override;

  WColor m_InitialColor;
  WColor m_NewColor;
};

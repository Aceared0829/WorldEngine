#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditorWidget.h>

#include <QWidget>

class QMouseEvent;

class W_GUIFOUNDATION_DLL WQtColorGradientEditorWidget : public QWidget, public Ui_ColorGradientEditorWidget
{
  Q_OBJECT

public:
  explicit WQtColorGradientEditorWidget(QWidget* pParent);
  ~WQtColorGradientEditorWidget();

  void SetColorGradient(const WColorGradient& gradient);
  const WColorGradient& GetColorGradient() const { return m_Gradient; }

  void ShowColorPicker() { on_ButtonColor_clicked(); }
  void SetScrubberPosition(WUInt64 uiTick);
  void SetScrubberPosition(WTime time);

  void FrameGradient();

Q_SIGNALS:
  void ColorCpAdded(double fPosX, const WColorGammaUB& color);
  void ColorCpMoved(WInt32 iIndex, float fNewPosX);
  void ColorCpDeleted(WInt32 iIndex);
  void ColorCpChanged(WInt32 iIndex, const WColorGammaUB& color);

  void AlphaCpAdded(double fPosX, WUInt8 uiAlpha);
  void AlphaCpMoved(WInt32 iIndex, double fNewPosX);
  void AlphaCpDeleted(WInt32 iIndex);
  void AlphaCpChanged(WInt32 iIndex, WUInt8 uiAlpha);

  void IntensityCpAdded(double fPosX, float fIntensity);
  void IntensityCpMoved(WInt32 iIndex, double fNewPosX);
  void IntensityCpDeleted(WInt32 iIndex);
  void IntensityCpChanged(WInt32 iIndex, float fIntensity);

  void NormalizeRange();

  void BeginOperation();
  void EndOperation(bool bCommit);

private Q_SLOTS:
  void on_ButtonFrame_clicked();
  void on_GradientWidget_selectionChanged(WInt32 colorCP, WInt32 alphaCP, WInt32 intensityCP);
  void on_SpinPosition_valueChanged(double value);
  void on_SpinPosition_editingFinished();
  void on_SpinAlpha_valueChanged(int value);
  void on_SliderAlpha_valueChanged(int value);
  void on_SliderAlpha_sliderPressed();
  void on_SliderAlpha_sliderReleased();
  void on_SpinIntensity_valueChanged(double value);
  void on_SpinIntensity_editingFinished();
  void on_ButtonColor_clicked();
  void onCurrentColorChanged(const WColor& col);
  void onColorAccepted();
  void onColorReset();
  void on_ButtonNormalize_clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  void UpdateCpUi();

  QPalette m_Pal;
  WInt32 m_iSelectedColorCP;
  WInt32 m_iSelectedAlphaCP;
  WInt32 m_iSelectedIntensityCP;
  WColorGradient m_Gradient;

  WColorGammaUB m_PickColorStart;
  WColorGammaUB m_PickColorCurrent;

  bool m_bTemporaryTransaction = false;
};

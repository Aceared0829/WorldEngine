#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <GuiFoundation/ui_ColorDialog.h>

class QLineEdit;
class WQtDoubleSpinBox;
class QPushButton;
class QSlider;


class W_GUIFOUNDATION_DLL WQtColorDialog : public WQtDialog, Ui_ColorDialog
{
  Q_OBJECT
public:
  WQtColorDialog(const WColor& initial, QWidget* pParent);
  ~WQtColorDialog();

  void ShowAlpha(bool bEnable);
  void ShowHDR(bool bEnable);

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

Q_SIGNALS:
  void CurrentColorChanged(const WColor& color);
  void ColorSelected(const WColor& color);

private Q_SLOTS:
  void ChangedRGB();
  void ChangedAlpha();
  void ChangedExposure();
  void ChangedHSV();
  void ChangedArea(double x, double y);
  void ChangedRange(double x);
  void ChangedHEX();

private:
  bool m_bAlpha;
  bool m_bHDR;

  float m_fHue;
  float m_fSaturation;
  float m_fValue;

  WUInt16 m_uiHue;
  WUInt8 m_uiSaturation;

  WUInt8 m_uiGammaRed;
  WUInt8 m_uiGammaGreen;
  WUInt8 m_uiGammaBlue;

  WUInt8 m_uiAlpha;
  float m_fExposureValue;

  WColor m_CurrentColor;

  static QByteArray s_LastDialogGeometry;

private:
  void ApplyColor();

  void RecomputeHDR();

  void ExtractColorRGB();
  void ExtractColorHSV();

  void ComputeRgbAndHsv(const WColor& color);
  void RecomputeRGB();
  void RecomputeHSV();
};

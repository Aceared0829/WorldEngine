#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

#include <QFrame>
#include <QLabel>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
class QToolButton;
class QMenu;
class WDocumentObject;
class WQtDoubleSpinBox;
class QSlider;

/// *** CHECKBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorCheckboxWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorCheckboxWidget();

  virtual void mousePressEvent(QMouseEvent* pEv) override;

private Q_SLOTS:
  void on_StateChanged_triggered(int state);

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const WVariant& value) override;

  QHBoxLayout* m_pLayout = nullptr;
  QCheckBox* m_pWidget = nullptr;
};



/// *** DOUBLE SPINBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorDoubleSpinboxWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorDoubleSpinboxWidget(WInt8 iNumComponents);

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  WInt8 m_iNumComponents = 0;
  WEnum<WVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget[4] = {};
};

/// *** TIME SPINBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorTimeWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorTimeWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget = nullptr;
};

/// *** ANGLE SPINBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorAngleWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorAngleWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget = nullptr;
};

/// *** INT SPINBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorIntSpinboxWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorIntSpinboxWidget(WInt8 iNumComponents, WInt32 iMinValue, WInt32 iMaxValue);
  ~WQtPropertyEditorIntSpinboxWidget();

  void SetReadOnly(bool bReadOnly = true) override;

private Q_SLOTS:
  void SlotValueChanged();
  void SlotSliderValueChanged(int value);
  void on_EditingFinished_triggered();
  void onBeginTemporary();
  void onEndTemporary();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  WInt8 m_iNumComponents = 0;
  WEnum<WVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget[4] = {};
  QSlider* m_pSlider = nullptr;
};

/// *** SLIDER ***

class W_GUIFOUNDATION_DLL WQtImageSliderWidget : public QWidget
{
  Q_OBJECT
public:
  using ImageGeneratorFunc = QImage (*)(WUInt32 uiWidth, WUInt32 uiHeight, double fMinValue, double fMaxValue);

  WQtImageSliderWidget(ImageGeneratorFunc generator, double fMinValue, double fMaxValue, QWidget* pParent);

  static WMap<WString, ImageGeneratorFunc> s_ImageGenerators;

  double GetValue() const { return m_fValue; }
  void SetValue(double fValue);

Q_SIGNALS:
  void valueChanged(double x);
  void sliderPressed();
  void sliderReleased();

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;
  virtual void mouseReleaseEvent(QMouseEvent*) override;

  void UpdateImage();

  ImageGeneratorFunc m_Generator = nullptr;
  QImage m_Image;
  double m_fValue = 0;
  double m_fMinValue = 0;
  double m_fMaxValue = 0;
};

class W_GUIFOUNDATION_DLL WQtPropertyEditorSliderWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorSliderWidget();
  ~WQtPropertyEditorSliderWidget();

private Q_SLOTS:
  void SlotSliderValueChanged(double fValue);
  void on_EditingFinished_triggered();
  void onBeginTemporary();
  void onEndTemporary();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bTemporaryCommand = false;
  WEnum<WVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  WQtImageSliderWidget* m_pSlider = nullptr;

  double m_fMinValue = 0;
  double m_fMaxValue = 0;
};

/// *** QUATERNION ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorQuaternionWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorQuaternionWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget[3] = {};
};

/// *** TRANSFORM ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorTransformWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorTransformWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  bool m_bTemporaryCommand = false;
  QVBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pWidget[9] = {};
};


/// *** LINEEDIT ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorLineEditWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorLineEditWidget();

  void SetReadOnly(bool bReadOnly = true) override;

protected Q_SLOTS:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  QHBoxLayout* m_pLayout = nullptr;
  QLineEdit* m_pWidget = nullptr;
  QLabel* m_pWarningIcon = nullptr;
  WEnum<WVariantType> m_OriginalType;
};


/// *** COLOR ***

class W_GUIFOUNDATION_DLL WQtColorButtonWidget : public QFrame
{
  Q_OBJECT

public:
  explicit WQtColorButtonWidget(QWidget* pParent);
  void SetColor(const WVariant& color);

Q_SIGNALS:
  void clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  QPalette m_Pal;
};

class W_GUIFOUNDATION_DLL WQtPropertyEditorColorWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorColorWidget();

private Q_SLOTS:
  void on_Button_triggered();
  void on_CurrentColor_changed(const WColor& color);
  void on_Color_reset();
  void on_Color_accepted();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  bool m_bExposeAlpha = false;
  bool m_bIsHDR = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtColorButtonWidget* m_pWidget = nullptr;
  WVariant m_OriginalValue;
};


/// *** ENUM COMBOBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorEnumWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorEnumWidget();

private Q_SLOTS:
  void on_CurrentEnum_changed(int iEnum);
  void on_ButtonClicked_changed(bool checked);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  QHBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pWidget = nullptr;
  WInt64 m_iCurrentEnum = 0;
  QPushButton* m_pButtons[2] = {nullptr, nullptr};
};


/// *** BITFLAGS COMBOBOX ***

class W_GUIFOUNDATION_DLL WQtPropertyEditorBitflagsWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorBitflagsWidget();
  virtual ~WQtPropertyEditorBitflagsWidget();

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;
  void SetAllChecked(bool bChecked);

protected:
  WMap<WInt64, QCheckBox*> m_Constants;
  QHBoxLayout* m_pLayout = nullptr;
  QPushButton* m_pWidget = nullptr;
  QPushButton* m_pAllButton = nullptr;
  QPushButton* m_pClearButton = nullptr;
  QMenu* m_pMenu = nullptr;
  WInt64 m_iCurrentBitflags = 0;
};


/// *** CURVE1D ***

class W_GUIFOUNDATION_DLL WQtCurve1DButtonWidget : public QLabel
{
  Q_OBJECT

public:
  explicit WQtCurve1DButtonWidget(QWidget* pParent);

  void UpdatePreview(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange);

Q_SIGNALS:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
};

class W_GUIFOUNDATION_DLL WQtPropertyEditorCurve1DWidget : public WQtPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorCurve1DWidget();

private Q_SLOTS:
  void on_Button_triggered();

protected:
  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  void UpdatePreview();
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  WQtCurve1DButtonWidget* m_pButton = nullptr;
  WCopyOnBroadcastEvent<const WDocumentObjectPropertyEvent&>::Unsubscriber m_Unsub;
  WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&>::Unsubscriber m_Unsub2;
};

/// *** COLOR GRADIENT ***

class W_GUIFOUNDATION_DLL WQtColorGradientButtonWidget : public QLabel
{
  Q_OBJECT

public:
  explicit WQtColorGradientButtonWidget(QWidget* pParent);

  void UpdatePreview(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pGradientObject);

Q_SIGNALS:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
};

class W_GUIFOUNDATION_DLL WQtPropertyEditorColorGradientWidget : public WQtPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorColorGradientWidget();

private Q_SLOTS:
  void on_Button_triggered();

protected:
  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  void UpdatePreview();
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void ObjectEventHandler(const WDocumentObjectEvent& e);


protected:
  QHBoxLayout* m_pLayout = nullptr;
  WQtColorGradientButtonWidget* m_pButton = nullptr;
  WCopyOnBroadcastEvent<const WDocumentObjectPropertyEvent&>::Unsubscriber m_Unsub;
  WEvent<const WDocumentObjectEvent&>::Unsubscriber m_Unsub2;
};

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/VarianceTypes.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class QSlider;

class WQtVarianceTypeWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtVarianceTypeWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();
  void SlotVarianceChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pValueWidget = nullptr;
  QSlider* m_pVarianceWidget = nullptr;
  const WAbstractMemberProperty* m_pValueProp = nullptr;
  const WAbstractMemberProperty* m_pVarianceProp = nullptr;
};

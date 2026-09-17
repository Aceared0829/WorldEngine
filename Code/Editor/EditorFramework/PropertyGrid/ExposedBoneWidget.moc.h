#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

class QSlider;

class WQtExposedBoneWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtExposedBoneWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDoubleSpinBox* m_pRotWidget[3];
};

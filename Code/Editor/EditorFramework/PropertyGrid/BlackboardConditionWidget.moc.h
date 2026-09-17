#pragma once

#include <Core/Utils/Blackboard.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class WQtDoubleSpinBox;
class WQtDynamicStringEnumMenuButton;

/// Displays all properties of WBlackboardCondition (entry name, operator and comparison value) in a single row.
///
/// WBlackboardCondition is a custom variant type, so the entire condition is edited as a single value. The entry
/// name reuses WQtDynamicStringEnumMenuButton with the "BlackboardKeysEnum" dynamic string enum that is registered
/// on the reflected property.
class W_EDITORFRAMEWORK_DLL WQtBlackboardConditionWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  WQtBlackboardConditionWidget();
  virtual ~WQtBlackboardConditionWidget();

protected Q_SLOTS:
  void onOperatorChanged(int iIndex);
  void onBeginTemporary();
  void onEndTemporary();
  void onValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  void SetEntryName(WStringView sName);
  void BroadcastCurrentValue();

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  WQtDynamicStringEnumMenuButton* m_pEntryButton = nullptr;
  QComboBox* m_pOperator = nullptr;
  WQtDoubleSpinBox* m_pComparisonValue = nullptr;
  WBlackboardCondition m_CurrentValue;
};

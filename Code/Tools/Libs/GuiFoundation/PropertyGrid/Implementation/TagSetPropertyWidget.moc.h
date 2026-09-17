#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class QCheckBox;

class W_GUIFOUNDATION_DLL WQtPropertyEditorTagSetWidget : public WQtPropertyWidget
{
  Q_OBJECT

public:
  WQtPropertyEditorTagSetWidget();
  virtual ~WQtPropertyEditorTagSetWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual bool HasLabel() const override { return true; }

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void onCheckBoxClicked(bool bChecked);
  void onRemoveInvalidTagsClicked();

private:
  virtual void OnInit() override;
  void InternalUpdateValue();

private:
  WDynamicArray<QCheckBox*> m_Tags;
  WDynamicArray<QAction*> m_InvalidTagActions; ///< Rebuilt every InternalUpdateValue(), since which tags are "invalid" depends on the current selection.
  QAction* m_pInvalidTagsAnchor = nullptr;      ///< Dynamic invalid-tag actions are inserted right before this one.
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
  WString m_sTagFilter;
};

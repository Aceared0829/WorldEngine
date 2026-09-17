#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>

class WManipulatorAttribute;
class QLabel;
class QToolButton;

class WQtManipulatorLabel : public QWidget
{
  Q_OBJECT
public:
  explicit WQtManipulatorLabel(QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  void setText(const QString& sText);
  void setAlignment(Qt::Alignment alignment);

  const WManipulatorAttribute* GetManipulator() const;
  void SetManipulator(const WManipulatorAttribute* pManipulator);

  bool GetManipulatorActive() const;
  void SetManipulatorActive(bool bActive);

  void SetSelection(const WArrayPtr<WPropertySelection>& items);

  void ToggleManipulator();

  void SetIsDefault(bool bIsDefault);

protected:
  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  QLabel* m_pLabel = nullptr;
  QToolButton* m_pButton = nullptr;
  WArrayPtr<WPropertySelection> m_Items;
  const WManipulatorAttribute* m_pManipulator = nullptr;
  QFont m_Font;
  bool m_bActive = false;
  bool m_bIsDefault = true;
};

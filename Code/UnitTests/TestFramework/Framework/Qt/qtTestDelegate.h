#pragma once

#ifdef W_USE_QT

#  include <QStyledItemDelegate>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class WQtTestFramework;

/// Delegate for WQtTestModel which shows bars for the test durations.
class W_TEST_DLL WQtTestDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  WQtTestDelegate(QObject* pParent);
  virtual ~WQtTestDelegate();

public: // QStyledItemDelegate interface
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif

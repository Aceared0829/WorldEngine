#pragma once

#ifdef W_USE_QT

#  include <QAbstractItemModel>
#  include <QDockWidget>
#  include <TestFramework/TestFrameworkDLL.h>
#  include <TestFramework/ui_qtLogMessageDock.h>
#  include <vector>

class WQtTestFramework;
struct WTestResultData;
class WQtLogMessageModel;
class WTestFrameworkResult;

/// Dock widget that lists the output of a given WResult struct.
class W_TEST_DLL WQtLogMessageDock : public QDockWidget, public Ui_qtLogMessageDock
{
  Q_OBJECT
public:
  WQtLogMessageDock(QObject* pParent, const WTestFrameworkResult* pResult);
  virtual ~WQtLogMessageDock();

public Q_SLOTS:
  void resetModel();
  void currentTestResultChanged(const WTestResultData* pTestResult);
  void currentTestSelectionChanged(const WTestResultData* pTestResult);

private:
  WQtLogMessageModel* m_pModel;
};

/// Model used by WQtLogMessageDock to list the output entries in WResult.
class W_TEST_DLL WQtLogMessageModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  WQtLogMessageModel(QObject* pParent, const WTestFrameworkResult* pResult);
  virtual ~WQtLogMessageModel();

  void resetModel();
  QModelIndex GetFirstIndexOfTestSelection();
  QModelIndex GetLastIndexOfTestSelection();

public Q_SLOTS:
  void currentTestResultChanged(const WTestResultData* pTestResult);
  void currentTestSelectionChanged(const WTestResultData* pTestResult);

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void UpdateVisibleEntries();

private:
  const WTestResultData* m_pCurrentTestSelection;
  const WTestFrameworkResult* m_pTestResult;
  std::vector<WUInt32> m_VisibleEntries;
  std::vector<WUInt8> m_VisibleEntriesIndention;
};

#endif

#pragma once

#ifdef W_USE_QT

#  include <QAbstractItemModel>
#  include <QColor>
#  include <QIcon>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class WQtTestFramework;

/// Helper class that stores the test hierarchy used in WQtTestModel.
class WQtTestModelEntry
{
public:
  WQtTestModelEntry(const WTestFrameworkResult* pResult, WInt32 iTestIndex = -1, WInt32 iSubTestIndex = -1);
  ~WQtTestModelEntry();

private:
  WQtTestModelEntry(WQtTestModelEntry&);
  void operator=(WQtTestModelEntry&);

public:
  enum WTestModelEntryType
  {
    RootNode,
    TestNode,
    SubTestNode
  };

  void ClearEntries();
  WUInt32 GetNumSubEntries() const;
  WQtTestModelEntry* GetSubEntry(WUInt32 uiIndex) const;
  void AddSubEntry(WQtTestModelEntry* pEntry);
  WQtTestModelEntry* GetParentEntry() const { return m_pParentEntry; }
  WUInt32 GetIndexInParent() const { return m_uiIndexInParent; }
  WTestModelEntryType GetNodeType() const;
  const WTestResultData* GetTestResult() const;
  WInt32 GetTestIndex() const { return m_iTestIndex; }
  WInt32 GetSubTestIndex() const { return m_iSubTestIndex; }

private:
  const WTestFrameworkResult* m_pResult;
  WInt32 m_iTestIndex;
  WInt32 m_iSubTestIndex;

  WQtTestModelEntry* m_pParentEntry = nullptr;
  WUInt32 m_uiIndexInParent = 0;
  std::deque<WQtTestModelEntry*> m_SubEntries;
};

/// A Model that lists all unit tests and sub-tests in a tree.
class W_TEST_DLL WQtTestModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  WQtTestModel(QObject* pParent, WQtTestFramework* pTestFramework);
  virtual ~WQtTestModel();

  void Reset();
  void InvalidateAll();
  void TestDataChanged(WInt32 iTestIndex, WInt32 iSubTestIndex);

  struct UserRoles
  {
    enum Enum
    {
      Duration = Qt::UserRole,
      DurationColor = Qt::UserRole + 1,
    };
  };

  struct Columns
  {
    enum Enum
    {
      Name = 0,
      Status,
      Duration,
      Errors,
      Asserts,
      Progress,
      ColumnCount,
    };
  };

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;

public Q_SLOTS:
  void UpdateModel();

private:
  WQtTestFramework* m_pTestFramework;
  WTestFrameworkResult* m_pResult;
  WQtTestModelEntry m_Root;
  QColor m_SucessColor;
  QColor m_FailedColor;
  QColor m_CustomStatusColor;
  QColor m_TestColor;
  QColor m_SubTestColor;
  QIcon m_TestIcon;
  QIcon m_TestIconOff;
};

#endif

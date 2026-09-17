#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Core/Console/Console.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/ui_CVarWidget.h>
#include <QItemDelegate>
#include <QPointer>
#include <QWidget>

class QStandardItemModel;
class QSortFilterProxyModel;
class WQtCVarModel;
class WQtCVarWidget;

class WQtCVarItemDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  explicit WQtCVarItemDelegate(QObject* pParent = nullptr)
    : QItemDelegate(pParent)
  {
  }

  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setEditorData(QWidget* pEditor, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

  WQtCVarModel* m_pModel = nullptr;

private:
  mutable QModelIndex m_Index;

private Q_SLOTS:
  void onComboChanged(int);
};

class WQtCVarModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  WQtCVarModel(WQtCVarWidget* pOwner);
  ~WQtCVarModel();

  void BeginResetModel();
  void EndResetModel();

public: // QAbstractItemModel interface
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

public:
  struct Entry
  {
    WString m_sFullName;
    QString m_sDisplayString;
    Entry* m_pParentEntry = nullptr;
    WDynamicArray<Entry*> m_ChildEntries;

    QString m_sPlugin;      // in which plugin a CVar is defined
    QString m_sDescription; // CVar description text
    WVariant m_Value;
  };

  Entry* CreateEntry(const char* szName);

  WQtCVarWidget* m_pOwner = nullptr;
  WDynamicArray<Entry*> m_RootEntries;
  WDeque<Entry> m_AllEntries;
};

/// Data used by WQtCVarWidget to represent CVar states
struct W_GUIFOUNDATION_DLL WCVarWidgetData
{
  mutable bool m_bNewEntry = true;

  WString m_sPlugin;      // in which plugin a CVar is defined
  WString m_sDescription; // CVar description text
  WUInt8 m_uiType = 0;    // WCVarType

  // 'union' over the different possible CVar types
  bool m_bValue = false;
  float m_fValue = 0.0f;
  WInt32 m_iValue = 0;
  WString m_sValue;
};

/// Displays CVar values in a table and allows to modify them.
class W_GUIFOUNDATION_DLL WQtCVarWidget : public QWidget, public Ui_CVarWidget
{
  Q_OBJECT

public:
  WQtCVarWidget(QWidget* pParent);
  ~WQtCVarWidget();

  /// Clears the table
  void Clear();

  /// Recreates the full UI. This is necessary when elements were added or removed.
  void RebuildCVarUI(const WMap<WString, WCVarWidgetData>& cvars);

  /// Updates the existing UI. This is sufficient if values changed only.
  void UpdateCVarUI(const WMap<WString, WCVarWidgetData>& cvars);

  void AddConsoleStrings(const WStringBuilder& sEncoded);

  WConsole& GetConsole() { return m_Console; }

Q_SIGNALS:
  void onBoolChanged(const char* szCVar, bool bNewValue);
  void onFloatChanged(const char* szCVar, float fNewValue);
  void onIntChanged(const char* szCVar, int iNewValue);
  void onStringChanged(const char* szCVar, const char* szNewValue);

private Q_SLOTS:
  void SearchTextChanged(const QString& text);
  void ConsoleEnterPressed();
  void ConsoleSpecialKeyPressed(Qt::Key key);

private:
  QPointer<WQtCVarModel> m_pItemModel;
  QPointer<QSortFilterProxyModel> m_pFilterModel;
  QPointer<WQtCVarItemDelegate> m_pItemDelegate;

  void OnConsoleEvent(const WConsoleEvent& e);

  WConsole m_Console;
};

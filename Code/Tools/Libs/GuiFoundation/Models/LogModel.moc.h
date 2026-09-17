#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QAbstractItemModel>

/// The Qt model that represents log output for a view
class W_GUIFOUNDATION_DLL WQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum UserRoles
  {
    Link = Qt::UserRole + 1,
    LinkText = Qt::UserRole + 2,
    LinkTarget = Qt::UserRole + 3,
  };

  WQtLogModel(QObject* pParent);
  void Clear();
  void SetLogLevel(WLogMsgType::Enum logLevel);
  void SetSearchText(const char* szText);
  void AddLogMsg(const WLogEntry& msg);

  WUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

  WUInt32 GetNumErrors() const { return m_uiNumErrors; }
  WUInt32 GetNumSeriousWarnings() const { return m_uiNumSeriousWarnings; }
  WUInt32 GetNumWarnings() const { return m_uiNumWarnings; }

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

Q_SIGNALS:
  void NewErrorsOrWarnings(const char* szLatest, bool bError);

private Q_SLOTS:
  /// Adds queued messages from a different thread to the model.
  void ProcessNewMessages();

private:
  /// Wraps a log entry with optional embedded link data parsed from [[text|target]] syntax.
  struct ModelEntry
  {
    WLogEntry m_Log;
    WStringView m_sLink;       ///< Full [[text|target]] substring, empty if no link present.
    WStringView m_sLinkText;   ///< Display text of the link.
    WStringView m_sLinkTarget; ///< Link target (e.g. "asset:{guid}").
  };

  void Invalidate();
  bool IsFiltered(const WLogEntry& lm) const;
  void UpdateVisibleEntries() const;
  void FindLink(ModelEntry& ref_entry) const;

  WLogMsgType::Enum m_LogLevel;
  WString m_sSearchText;
  WDeque<ModelEntry> m_AllMessages;

  mutable bool m_bIsValid;
  mutable WDeque<const ModelEntry*> m_VisibleMessages;
  mutable WHybridArray<const ModelEntry*, 16> m_BlockQueue;

  mutable WMutex m_NewMessagesMutex;
  WDeque<WLogEntry> m_NewMessages;
  bool m_bProcessingPending = false; ///< Prevents duplicate queued ProcessNewMessages invocations

  WUInt32 m_uiNumErrors = 0;
  WUInt32 m_uiNumSeriousWarnings = 0;
  WUInt32 m_uiNumWarnings = 0;
};

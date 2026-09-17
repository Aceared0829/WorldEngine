#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WCommandHistory;

class W_TOOLSFOUNDATION_DLL WCommandTransaction : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WCommandTransaction, WCommand);

public:
  WCommandTransaction();
  ~WCommandTransaction();

  WString m_sDisplayString;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;
  WStatus AddCommandTransaction(WCommand* command);

private:
  friend class WCommandHistory;
};

struct WCommandHistoryEvent
{
  enum class Type
  {
    UndoStarted,
    UndoEnded,
    RedoStarted,
    RedoEnded,
    TransactionStarted,        ///< Emit after initial transaction started.
    BeforeTransactionEnded,    ///< Emit before initial transaction ended.
    BeforeTransactionCanceled, ///< Emit before initial transaction ended.
    TransactionEnded,          ///< Emit after initial transaction ended.
    TransactionCanceled,       ///< Emit after initial transaction canceled.
    HistoryChanged,
  };

  Type m_Type;
  const WDocument* m_pDocument;
};

/// Stores the undo / redo stacks of transactions done on a document.
class W_TOOLSFOUNDATION_DLL WCommandHistory
{
public:
  WEvent<const WCommandHistoryEvent&, WMutex> m_Events;

  // Storage for the command history so it can be swapped when using multiple sub documents.
  class Storage : public WRefCounted
  {
  public:
    WHybridArray<WCommandTransaction*, 4> m_TransactionStack;
    WHybridArray<WCommand*, 4> m_ActiveCommandStack;
    WDeque<WCommandTransaction*> m_UndoHistory;
    WDeque<WCommandTransaction*> m_RedoHistory;
    WDocument* m_pDocument = nullptr;
    WEvent<const WCommandHistoryEvent&, WMutex> m_Events;

    /// The undo stack size at the point when the document was last saved. -1 if the saved state is no longer reachable via undo/redo.
    WInt32 m_iSavedHistoryIndex = 0;
  };

public:
  WCommandHistory(WDocument* pDocument);
  ~WCommandHistory();

  const WDocument* GetDocument() const { return m_pHistoryStorage->m_pDocument; }

  WStatus Undo(WUInt32 uiNumEntries = 1);
  WStatus Redo(WUInt32 uiNumEntries = 1);

  bool CanUndo() const;
  bool CanRedo() const;

  WStringView GetUndoDisplayString() const;
  WStringView GetRedoDisplayString() const;

  void StartTransaction(const WFormatString& displayString);
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_pHistoryStorage->m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  /// Call this to start a series of transactions that typically change the same value over and over (e.g. dragging an object to a position).
  /// Every time a new transaction is started, the previous one is undone first. At the end of a series of temporary transactions, only the last
  /// transaction will be stored as a single undo step. Call this first and then start a transaction inside it.
  void BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  void CancelTemporaryCommands();
  void FinishTemporaryCommands();

  bool InTemporaryTransaction() const;
  void SuspendTemporaryTransaction();
  void ResumeTemporaryTransaction();

  WStatus AddCommand(WCommand& ref_command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

  WUInt32 GetUndoStackSize() const;
  WUInt32 GetRedoStackSize() const;
  const WCommandTransaction* GetUndoStackEntry(WUInt32 uiIndex) const;
  const WCommandTransaction* GetRedoStackEntry(WUInt32 uiIndex) const;

  WSharedPtr<WCommandHistory::Storage> SwapStorage(WSharedPtr<WCommandHistory::Storage> pNewStorage);
  WSharedPtr<WCommandHistory::Storage> GetStorage() { return m_pHistoryStorage; }

private:
  friend class WCommand;

  WStatus UndoInternal();
  WStatus RedoInternal();

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  WSharedPtr<WCommandHistory::Storage> m_pHistoryStorage;

  WEvent<const WCommandHistoryEvent&, WMutex>::Unsubscriber m_EventsUnsubscriber;
  WEvent<const WDocumentEvent&>::Unsubscriber m_DocumentSavedUnsubscriber;

  bool m_bFireEventsWhenUndoingTempCommands = false;
  bool m_bTemporaryMode = false;
  WInt32 m_iTemporaryDepth = -1;
  WInt32 m_iPreSuspendTemporaryDepth = -1;
  bool m_bIsInUndoRedo = false;
};

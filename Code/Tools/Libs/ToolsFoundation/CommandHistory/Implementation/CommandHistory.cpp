#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCommandTransaction, 1, WRTTIDefaultAllocator<WCommandTransaction>)
W_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// WCommandTransaction
////////////////////////////////////////////////////////////////////////

WCommandTransaction::WCommandTransaction()
{
  // doesn't do anything on its own
  m_bModifiedDocument = false;
}

WCommandTransaction::~WCommandTransaction()
{
  W_ASSERT_DEV(m_ChildActions.IsEmpty(), "The list should be cleared in 'Cleanup'");
}

WStatus WCommandTransaction::DoInternal(bool bRedo)
{
  W_ASSERT_DEV(bRedo == true, "Implementation error");
  return WStatus(W_SUCCESS);
}

WStatus WCommandTransaction::UndoInternal(bool bFireEvents)
{
  return WStatus(W_SUCCESS);
}

void WCommandTransaction::CleanupInternal(CommandState state) {}

WStatus WCommandTransaction::AddCommandTransaction(WCommand* pCommand)
{
  pCommand->m_pDocument = m_pDocument;
  m_ChildActions.PushBack(pCommand);
  return WStatus(W_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// WCommandHistory
////////////////////////////////////////////////////////////////////////

WCommandHistory::WCommandHistory(WDocument* pDocument)
{
  auto pStorage = W_DEFAULT_NEW(Storage);
  pStorage->m_pDocument = pDocument;
  SwapStorage(pStorage);

  m_bTemporaryMode = false;
  m_bIsInUndoRedo = false;
}

WCommandHistory::~WCommandHistory()
{
  if (m_pHistoryStorage->GetRefCount() == 1)
  {
    W_ASSERT_ALWAYS(m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
    W_ASSERT_ALWAYS(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
  }
}

void WCommandHistory::BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands)
{
  W_ASSERT_DEV(!m_bTemporaryMode, "Temporary Mode cannot be nested");
  StartTransaction(sDisplayString);
  StartTransaction("[Temporary]");

  m_bFireEventsWhenUndoingTempCommands = bFireEventsWhenUndoingTempCommands;
  m_bTemporaryMode = true;
  m_iTemporaryDepth = (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
}

void WCommandHistory::CancelTemporaryCommands()
{
  EndTemporaryCommands(true);
  EndTransaction(true);
}

void WCommandHistory::FinishTemporaryCommands()
{
  EndTemporaryCommands(false);
  EndTransaction(false);
}

bool WCommandHistory::InTemporaryTransaction() const
{
  return m_bTemporaryMode;
}


void WCommandHistory::SuspendTemporaryTransaction()
{
  m_iPreSuspendTemporaryDepth = (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
  W_ASSERT_DEV(m_bTemporaryMode, "No temporary transaction active.");
  while (m_iTemporaryDepth < (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    EndTransaction(true);
  }
  EndTemporaryCommands(true);
}

void WCommandHistory::ResumeTemporaryTransaction()
{
  W_ASSERT_DEV(m_iTemporaryDepth == (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount() + 1, "Can't resume temporary, not before temporary depth.");
  while (m_iPreSuspendTemporaryDepth > (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    StartTransaction("[Temporary]");
  }
  m_bTemporaryMode = true;
  W_ASSERT_DEV(m_iPreSuspendTemporaryDepth == (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "");
}

void WCommandHistory::EndTemporaryCommands(bool bCancel)
{
  W_ASSERT_DEV(m_bTemporaryMode, "Temporary Mode was not enabled");
  W_ASSERT_DEV(m_iTemporaryDepth == (WInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "Transaction stack is at depth {0} but temporary is at {1}",
    m_pHistoryStorage->m_TransactionStack.GetCount(), m_iTemporaryDepth);
  m_bTemporaryMode = false;

  EndTransaction(bCancel);
}

WStatus WCommandHistory::UndoInternal()
{
  W_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  W_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  W_ASSERT_DEV(!m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = WCommandHistoryEvent::Type::UndoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  WCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

  WStatus status = pTransaction->Undo(true);
  if (status.Succeeded())
  {
    m_pHistoryStorage->m_UndoHistory.PopBack();
    m_pHistoryStorage->m_RedoHistory.PushBack(pTransaction);

    const bool bAtSavedState = m_pHistoryStorage->m_iSavedHistoryIndex >= 0 &&
                               (WInt32)m_pHistoryStorage->m_UndoHistory.GetCount() == m_pHistoryStorage->m_iSavedHistoryIndex;
    m_pHistoryStorage->m_pDocument->SetModified(!bAtSavedState);

    status = WStatus(W_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = WCommandHistoryEvent::Type::UndoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

WStatus WCommandHistory::Undo(WUInt32 uiNumEntries)
{
  for (WUInt32 i = 0; i < uiNumEntries; i++)
  {
    W_SUCCEED_OR_RETURN(UndoInternal());
  }

  return WStatus(W_SUCCESS);
}

WStatus WCommandHistory::RedoInternal()
{
  W_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  W_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  W_ASSERT_DEV(!m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = WCommandHistoryEvent::Type::RedoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  WCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

  WStatus status(W_FAILURE);
  if (pTransaction->Do(true).Succeeded())
  {
    m_pHistoryStorage->m_RedoHistory.PopBack();
    m_pHistoryStorage->m_UndoHistory.PushBack(pTransaction);

    const bool bAtSavedState = m_pHistoryStorage->m_iSavedHistoryIndex >= 0 &&
                               (WInt32)m_pHistoryStorage->m_UndoHistory.GetCount() == m_pHistoryStorage->m_iSavedHistoryIndex;
    m_pHistoryStorage->m_pDocument->SetModified(!bAtSavedState);

    status = WStatus(W_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = WCommandHistoryEvent::Type::RedoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

WStatus WCommandHistory::Redo(WUInt32 uiNumEntries)
{
  for (WUInt32 i = 0; i < uiNumEntries; i++)
  {
    W_SUCCEED_OR_RETURN(RedoInternal());
  }

  return WStatus(W_SUCCESS);
}

bool WCommandHistory::CanUndo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_UndoHistory.IsEmpty();
}

bool WCommandHistory::CanRedo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_RedoHistory.IsEmpty();
}


WStringView WCommandHistory::GetUndoDisplayString() const
{
  if (m_pHistoryStorage->m_UndoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_UndoHistory.PeekBack()->m_sDisplayString;
}


WStringView WCommandHistory::GetRedoDisplayString() const
{
  if (m_pHistoryStorage->m_RedoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_RedoHistory.PeekBack()->m_sDisplayString;
}

void WCommandHistory::StartTransaction(const WFormatString& displayString)
{
  W_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot start new transaction while redoing/undoing.");

  /// \todo Allow to have a limited transaction history and clean up transactions after a while

  WCommandTransaction* pTransaction;

  if (m_bTemporaryMode && !m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();
    pTransaction->Undo(m_bFireEventsWhenUndoingTempCommands).IgnoreResult();
    pTransaction->Cleanup(WCommand::CommandState::WasUndone);
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    return;
  }

  WStringBuilder tmp;

  pTransaction = WGetStaticRTTI<WCommandTransaction>()->GetAllocator()->Allocate<WCommandTransaction>();
  pTransaction->m_pDocument = m_pHistoryStorage->m_pDocument;
  pTransaction->m_sDisplayString = displayString.GetText(tmp);

  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // Stacked transaction
    m_pHistoryStorage->m_TransactionStack.PeekBack()->AddCommandTransaction(pTransaction).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
  }
  else
  {
    // Initial transaction
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    {
      WCommandHistoryEvent e;
      e.m_pDocument = m_pHistoryStorage->m_pDocument;
      e.m_Type = WCommandHistoryEvent::Type::TransactionStarted;
      m_pHistoryStorage->m_Events.Broadcast(e);
    }
  }
  return;
}

void WCommandHistory::EndTransaction(bool bCancel)
{
  W_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

  if (m_pHistoryStorage->m_TransactionStack.GetCount() == 1)
  {
    /// Empty transactions are always canceled, so that they do not create an unnecessary undo action and clear the redo stack

    const bool bDidAnything = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasChildActions();
    if (!bDidAnything)
      bCancel = true;

    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = bCancel ? WCommandHistoryEvent::Type::BeforeTransactionCanceled : WCommandHistoryEvent::Type::BeforeTransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  if (!bCancel)
  {
    if (m_pHistoryStorage->m_TransactionStack.GetCount() > 1)
    {
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
    }
    else
    {
      const bool bDidModifyDoc = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasModifiedDocument();
      m_pHistoryStorage->m_UndoHistory.PushBack(m_pHistoryStorage->m_TransactionStack.PeekBack());
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
      ClearRedoHistory();

      if (bDidModifyDoc)
      {
        m_pHistoryStorage->m_pDocument->SetModified(true);
      }
    }
  }
  else
  {
    WCommandTransaction* pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();

    pTransaction->Undo(true).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PopBack();
    m_pHistoryStorage->m_ActiveCommandStack.PopBack();

    if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
    {
      pTransaction->Cleanup(WCommand::CommandState::WasUndone);
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // All transactions done
    WCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = bCancel ? WCommandHistoryEvent::Type::TransactionCanceled : WCommandHistoryEvent::Type::TransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
}

WStatus WCommandHistory::AddCommand(WCommand& ref_command)
{
  W_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Cannot add command while no transaction is started");
  W_ASSERT_DEV(!m_pHistoryStorage->m_ActiveCommandStack.IsEmpty(), "Transaction stack is not synced anymore with m_ActiveCommandStack");

  auto res = m_pHistoryStorage->m_ActiveCommandStack.PeekBack()->AddSubCommand(ref_command);

  // Error handling should be on the caller side.
  // if (res.Failed() && !res.m_sMessage.IsEmpty())
  //{
  //  WLog::Error("Command failed: '{0}'", res.m_sMessage);
  //}

  return res;
}

void WCommandHistory::ClearUndoHistory()
{
  W_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_UndoHistory.IsEmpty())
  {
    WCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

    pTransaction->Cleanup(WCommand::CommandState::WasDone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_UndoHistory.PopBack();
  }

  // The saved state may no longer be reachable if it was in the undo history.
  if (m_pHistoryStorage->m_iSavedHistoryIndex > 0)
  {
    m_pHistoryStorage->m_iSavedHistoryIndex = -1;
  }
}

void WCommandHistory::ClearRedoHistory()
{
  W_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_RedoHistory.IsEmpty())
  {
    WCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

    pTransaction->Cleanup(WCommand::CommandState::WasUndone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_RedoHistory.PopBack();
  }

  // If the saved state is at or beyond the current undo position, it was in the redo stack and is now unreachable.
  if (m_pHistoryStorage->m_iSavedHistoryIndex >= (WInt32)m_pHistoryStorage->m_UndoHistory.GetCount())
  {
    m_pHistoryStorage->m_iSavedHistoryIndex = -1;
  }
}

void WCommandHistory::MergeLastTwoTransactions()
{
  /// \todo This would not be necessary, if hierarchical transactions would not crash

  W_ASSERT_DEV(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "This can only be called directly after EndTransaction, when the redo history is empty");
  W_ASSERT_DEV(m_pHistoryStorage->m_UndoHistory.GetCount() >= 2, "Can only do this when at least two transcations are in the queue");

  WCommandTransaction* pLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  m_pHistoryStorage->m_UndoHistory.PopBack();

  WCommandTransaction* pNowLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  pNowLast->m_ChildActions.PushBackRange(pLast->m_ChildActions);

  pLast->m_ChildActions.Clear();

  pLast->GetDynamicRTTI()->GetAllocator()->Deallocate(pLast);
}

WUInt32 WCommandHistory::GetUndoStackSize() const
{
  return m_pHistoryStorage->m_UndoHistory.GetCount();
}

WUInt32 WCommandHistory::GetRedoStackSize() const
{
  return m_pHistoryStorage->m_RedoHistory.GetCount();
}

const WCommandTransaction* WCommandHistory::GetUndoStackEntry(WUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_UndoHistory[GetUndoStackSize() - 1 - uiIndex];
}

const WCommandTransaction* WCommandHistory::GetRedoStackEntry(WUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_RedoHistory[GetRedoStackSize() - 1 - uiIndex];
}

WSharedPtr<WCommandHistory::Storage> WCommandHistory::SwapStorage(WSharedPtr<WCommandHistory::Storage> pNewStorage)
{
  W_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  W_ASSERT_DEV(!m_bIsInUndoRedo, "Can't be in Undo/Redo when swapping storage.");

  auto retVal = m_pHistoryStorage;

  m_EventsUnsubscriber.Unsubscribe();
  m_DocumentSavedUnsubscriber.Unsubscribe();

  m_pHistoryStorage = pNewStorage;

  m_pHistoryStorage->m_Events.AddEventHandler([this](const WCommandHistoryEvent& e)
    { m_Events.Broadcast(e); },
    m_EventsUnsubscriber);

  m_pHistoryStorage->m_pDocument->m_EventsOne.AddEventHandler(
    [this](const WDocumentEvent& e)
    {
      if (e.m_Type == WDocumentEvent::Type::DocumentSaved)
      {
        m_pHistoryStorage->m_iSavedHistoryIndex = (WInt32)m_pHistoryStorage->m_UndoHistory.GetCount();
      }
    },
    m_DocumentSavedUnsubscriber);

  return retVal;
}

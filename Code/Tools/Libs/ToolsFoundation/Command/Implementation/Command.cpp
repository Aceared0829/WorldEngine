#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCommand, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WCommand::WCommand() = default;
WCommand::~WCommand() = default;

bool WCommand::HasModifiedDocument() const
{
  if (m_bModifiedDocument)
    return true;

  for (const auto& ca : m_ChildActions)
  {
    if (ca->HasModifiedDocument())
      return true;
  }

  return false;
}

WStatus WCommand::Do(bool bRedo)
{
  WStatus status = DoInternal(bRedo);
  if (status.Failed())
  {
    if (bRedo)
    {
      // A command that originally succeeded failed on redo!
      return status;
    }
    else
    {
      for (WInt32 j = m_ChildActions.GetCount() - 1; j >= 0; --j)
      {
        WStatus status2 = m_ChildActions[j]->Undo(true);
        W_ASSERT_DEV(status2.Succeeded(), "Failed do could not be recovered! Inconsistent state!");
      }
      return status;
    }
  }
  if (!bRedo)
    return W_SUCCESS;

  const WUInt32 uiChildActions = m_ChildActions.GetCount();
  for (WUInt32 i = 0; i < uiChildActions; ++i)
  {
    status = m_ChildActions[i]->Do(bRedo);
    if (status.Failed())
    {
      for (WInt32 j = i - 1; j >= 0; --j)
      {
        WStatus status2 = m_ChildActions[j]->Undo(true);
        W_ASSERT_DEV(status2.Succeeded(), "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return status;
    }
  }
  return W_SUCCESS;
}

WStatus WCommand::Undo(bool bFireEvents)
{
  const WUInt32 uiChildActions = m_ChildActions.GetCount();
  for (WInt32 i = uiChildActions - 1; i >= 0; --i)
  {
    WStatus status = m_ChildActions[i]->Undo(bFireEvents);
    if (status.Failed())
    {
      for (WUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        WStatus status2 = m_ChildActions[j]->Do(true);
        W_ASSERT_DEV(status2.Succeeded(), "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on undo!
      return status;
    }
  }

  WStatus status = UndoInternal(bFireEvents);
  if (status.Failed())
  {
    for (WUInt32 j = 0; j < uiChildActions; ++j)
    {
      WStatus status2 = m_ChildActions[j]->Do(true);
      W_ASSERT_DEV(status2.Succeeded(), "Failed undo could not be recovered! Inconsistent state!");
    }
    // A command that originally succeeded failed on undo!
    return status;
  }

  return W_SUCCESS;
}

void WCommand::Cleanup(CommandState state)
{
  CleanupInternal(state);

  for (WCommand* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }

  m_ChildActions.Clear();
}


WStatus WCommand::AddSubCommand(WCommand& command)
{
  WCommand* pCommand = WReflectionSerializer::Clone(&command);
  const WRTTI* pRtti = pCommand->GetDynamicRTTI();

  pCommand->m_pDocument = m_pDocument;

  m_ChildActions.PushBack(pCommand);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PushBack(pCommand);
  WStatus ret = pCommand->Do(false);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PopBack();

  if (ret.Failed())
  {
    m_ChildActions.PopBack();
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  if (pCommand->HasReturnValues())
  {
    // Write properties back so any return values get written.
    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WReflectionSerializer::WriteObjectToBinary(writer, pCommand->GetDynamicRTTI(), pCommand);
    WReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, &command);
  }

  return W_SUCCESS;
}

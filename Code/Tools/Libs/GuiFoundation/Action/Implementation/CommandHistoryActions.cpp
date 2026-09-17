#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCommandHistoryAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WCommandHistoryActions::s_hCommandHistoryCategory;
WActionDescriptorHandle WCommandHistoryActions::s_hUndo;
WActionDescriptorHandle WCommandHistoryActions::s_hRedo;

void WCommandHistoryActions::RegisterActions()
{
  s_hCommandHistoryCategory = W_REGISTER_CATEGORY("CmdHistoryCategory");
  s_hUndo = W_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Undo", WActionScope::Document, "Document", "Ctrl+Z", WCommandHistoryAction, WCommandHistoryAction::ButtonType::Undo);
  s_hRedo = W_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Redo", WActionScope::Document, "Document", "Ctrl+Y", WCommandHistoryAction, WCommandHistoryAction::ButtonType::Redo);
}

void WCommandHistoryActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCommandHistoryCategory);
  WActionManager::UnregisterAction(s_hUndo);
  WActionManager::UnregisterAction(s_hRedo);
}

void WCommandHistoryActions::MapActions(WStringView sMapping, WStringView sTargetMenu)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCommandHistoryCategory, sTargetMenu, 3.0f);
  pMap->MapAction(s_hUndo, sTargetMenu, "CmdHistoryCategory", 1.0f);
  pMap->MapAction(s_hRedo, sTargetMenu, "CmdHistoryCategory", 2.0f);
}

WCommandHistoryAction::WCommandHistoryAction(const WActionContext& context, const char* szName, ButtonType button)
  : WDynamicActionAndMenuAction(context, szName, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case WCommandHistoryAction::ButtonType::Undo:
      SetIconPath(":/GuiFoundation/Icons/Undo.svg");
      break;
    case WCommandHistoryAction::ButtonType::Redo:
      SetIconPath(":/GuiFoundation/Icons/Redo.svg");
      break;
  }

  m_Context.m_pDocument->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WCommandHistoryAction::CommandHistoryEventHandler, this));

  UpdateState();
}

WCommandHistoryAction::~WCommandHistoryAction()
{
  m_Context.m_pDocument->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WCommandHistoryAction::CommandHistoryEventHandler, this));
}

void WCommandHistoryAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  WCommandHistory* pHistory = m_Context.m_pDocument->GetCommandHistory();

  const WUInt32 iCount = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackSize() : pHistory->GetRedoStackSize();
  for (WUInt32 i = 0; i < iCount; i++)
  {
    const WCommandTransaction* pTransaction = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackEntry(i) : pHistory->GetRedoStackEntry(i);
    WDynamicMenuAction::Item entryItem;
    entryItem.m_sDisplay = pTransaction->m_sDisplayString;
    entryItem.m_UserValue = (WUInt32)i + 1; // Number of steps to undo / redo.
    out_entries.PushBack(entryItem);
  }
}

void WCommandHistoryAction::Execute(const WVariant& value)
{
  WUInt32 iCount = value.IsValid() ? value.ConvertTo<WUInt32>() : 1;

  switch (m_ButtonType)
  {
    case ButtonType::Undo:
    {
      W_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanUndo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Undo(iCount);
      WQtUiServices::MessageBoxStatus(stat, "Could not execute the Undo operation");
    }
    break;

    case ButtonType::Redo:
    {
      W_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanRedo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Redo(iCount);
      WQtUiServices::MessageBoxStatus(stat, "Could not execute the Redo operation");
    }
    break;
  }
}

void WCommandHistoryAction::UpdateState()
{
  switch (m_ButtonType)
  {
    case ButtonType::Undo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetUndoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanUndo());
      break;

    case ButtonType::Redo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetRedoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanRedo());
      break;
  }
}

void WCommandHistoryAction::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  UpdateState();
}

#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GameEngine/Console/ConsoleActions.h>

WDynamicArray<WConsoleActions::WConsoleActionsDesc> WConsoleActions::s_ConsoleActions;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GameEngine, ConsoleActions)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_SHUTDOWN
  {
    WConsoleActions::ClearActions();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  WInt32 CompareConsoleActions(const WConsoleActions::WConsoleActionsDesc& lhs, const WConsoleActions::WConsoleActionsDesc& rhs)
  {
    const WInt32 iMenuCompare = lhs.m_sMenu.Compare(rhs.m_sMenu);
    if (iMenuCompare != 0)
      return iMenuCompare;

    return lhs.m_sAction.Compare(rhs.m_sAction);
  }
} // namespace

void WConsoleActions::AddAction(WStringView sInputSet, WStringView sAction, WStringView sMenu, Action action)
{
  WConsoleActionsDesc desc;
  desc.m_sInputSet = sInputSet;
  desc.m_sAction = sAction;
  desc.m_sMenu = sMenu;
  desc.m_Action = std::move(action);

  RemoveAction(sInputSet, sAction);

  WUInt32 uiInsertIndex = 0;
  while (uiInsertIndex < s_ConsoleActions.GetCount() && CompareConsoleActions(s_ConsoleActions[uiInsertIndex], desc) <= 0)
  {
    ++uiInsertIndex;
  }

  s_ConsoleActions.InsertAt(uiInsertIndex, std::move(desc));
}

void WConsoleActions::RemoveAction(WStringView sInputSet, WStringView sAction)
{
  for (WUInt32 uiActionIndex = 0; uiActionIndex < s_ConsoleActions.GetCount(); ++uiActionIndex)
  {
    if (s_ConsoleActions[uiActionIndex].m_sInputSet.IsEqual(sInputSet) && s_ConsoleActions[uiActionIndex].m_sAction.IsEqual(sAction))
    {
      s_ConsoleActions.RemoveAtAndCopy(uiActionIndex);
      return;
    }
  }
}

void WConsoleActions::HandleInput()
{
  for (auto& desc : s_ConsoleActions)
  {
    if (WInputManager::GetInputActionState(desc.m_sInputSet, desc.m_sAction) == WKeyState::Pressed && desc.m_Action.IsValid())
    {
      desc.m_Action();
    }
  }
}

WArrayPtr<const WConsoleActions::WConsoleActionsDesc> WConsoleActions::GetActions()
{
  return s_ConsoleActions;
}

void WConsoleActions::ClearActions()
{
  s_ConsoleActions.Clear();
}

W_STATICLINK_FILE(GameEngine, GameEngine_Console_Implementation_ConsoleActions);

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GameEngine/GameEngineDLL.h>

// Allows to add a simple action that is shown in the console menu.
// To execute shortcuts, call `HandleInput` during your application's 'Run_ProcessApplicationInput' function.
class W_GAMEENGINE_DLL WConsoleActions
{
public:
  using Action = WDelegate<void()>;

  struct WConsoleActionsDesc
  {
    WString m_sInputSet;
    WString m_sAction;
    WString m_sMenu;
    Action m_Action;
  };

  // Adds an action. sInputSet and sAction should match what was passed into 'WInputManager::SetInputActionConfig' and uniquely identifies the action. Actions are sorted by their menu and action name.
  static void AddAction(WStringView sInputSet, WStringView sAction, WStringView sMenu, Action action);
  // Removes an action.
  static void RemoveAction(WStringView sInputSet, WStringView sAction);

  // Executes any action who's shortcut is 'WKeyState::Pressed'.
  static void HandleInput();
  // Returns all actions sorted by menu first, then by action.
  static WArrayPtr<const WConsoleActionsDesc> GetActions();
  static void ClearActions();

private:
  static WDynamicArray<WConsoleActionsDesc> s_ConsoleActions;
};
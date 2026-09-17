#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

/// Registers an WAction whose constructor takes no arguments.
#define W_REGISTER_ACTION_0(ActionName, Scope, CategoryName, ShortCut, ActionClass)                                  \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(ActionClass, context, ActionName); }));

/// Registers an WAction whose constructor takes one argument.
#define W_REGISTER_ACTION_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                          \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

/// Registers an WAction whose constructor takes two arguments.
#define W_REGISTER_ACTION_2(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1, Param2)                  \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(ActionClass, context, ActionName, Param1, Param2); }));

/// Registers an WDynamicMenuAction
#define W_REGISTER_DYNAMIC_MENU(ActionName, ActionClass, IconPath)                                                  \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Menu, WActionScope::Default, ActionName, "", "", \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(ActionClass, context, ActionName, IconPath); }));

/// Registers an WDynamicActionAndMenuAction.
#define W_REGISTER_ACTION_AND_DYNAMIC_MENU_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::ActionAndMenu, Scope, ActionName, CategoryName, ShortCut, \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

/// Registers a category that should be treated as a sub-menu.
#define W_REGISTER_MENU(ActionName)                                                                                 \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Menu, WActionScope::Default, ActionName, "", "", \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(WMenuAction, context, ActionName, ""); }));

/// Registers a category that should be treated as a sub-menu and specifies a custom QIcon path.
#define W_REGISTER_MENU_WITH_ICON(ActionName, IconPath)                                                             \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Menu, WActionScope::Default, ActionName, "", "", \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(WMenuAction, context, ActionName, IconPath); }));

/// Registers a category that should just be a grouped area in a menu, but no dedicated sub-menu.
#define W_REGISTER_CATEGORY(CategoryName)                                                                                 \
  WActionManager::RegisterAction(WActionDescriptor(WActionType::Category, WActionScope::Default, CategoryName, "", "", \
    [](const WActionContext& context) -> WAction* { return W_DEFAULT_NEW(WCategoryAction, context); }));

/// Stores 'actions' (things that can be triggered from UI).
///
/// Actions are usually represented by a button in a toolbar, or a menu entry.
/// Actions are unique across the entire application. Each action is registered exactly once,
/// but it may be referenced by many different WActionMap instances, which defines how an action shows up in a window.
///
/// Through RegisterAction() / UnregisterAction() an action is added or removed.
/// These functions are usually not called directly, but rather the macros at the top of this file are used (see W_REGISTER_CATEGORY, W_REGISTER_MENU, W_REGISTER_ACTION_X, ...).
///
/// Unit tests can call ExecuteAction() to directly invoke an action.
/// Widgets use WActionMap to organize which actions are available in a window, and how they are structured.
/// For instance, the same action can appear in a menu, in a toolbar and a context menu. In each case their location may be different (top-level, in a sub-menu, etc).
/// See WActionMap for details.
class W_GUIFOUNDATION_DLL WActionManager
{
public:
  static WActionDescriptorHandle RegisterAction(const WActionDescriptor& desc);
  static bool UnregisterAction(WActionDescriptorHandle& ref_hAction);
  static const WActionDescriptor* GetActionDescriptor(WActionDescriptorHandle hAction);
  static WActionDescriptorHandle GetActionHandle(WStringView sCategory, WStringView sActionName);

  /// Searches all action categories for the given action name. Returns the category name in which the action name was found, or an empty
  /// string.
  static WString FindActionCategory(WStringView sActionName);

  /// Quick way to execute an action from code
  ///
  /// The use case is mostly for unit tests, which need to execute actions directly and without a link dependency on
  /// the code that registered the action.
  ///
  /// \param szCategory The category of the action, ie. under which name the action appears in the Shortcut binding dialog.
  ///        For example "Scene", "Scene - Cameras", "Scene - Selection", "Assets" etc.
  ///        This parameter may be nullptr in which case FindActionCategory(szActionName) is used to try to detect the category automatically.
  /// \param szActionName The name (not mapped path) under which the action was registered.
  ///        For example "Selection.Copy", "Prefabs.ConvertToEngine", "Scene.Camera.SnapObjectToCamera"
  /// \param context The context in which to execute the action. Depending on the WActionScope of the target action,
  ///        some members are optional. E.g. for document actions, only the m_pDocument member must be specified.
  /// \param value Optional value passed through to the WAction::Execute() call. Some actions use it, most don't.
  /// \return Returns failure in case the action could not be found.
  static WResult ExecuteAction(WStringView sCategory, WStringView sActionName, const WActionContext& context, const WVariant& value = WVariant());

  static void SaveShortcutAssignment();
  static void LoadShortcutAssignment();

  static const WIdTable<WActionId, WActionDescriptor*>::ConstIterator GetActionIterator();

  struct Event
  {
    enum class Type
    {
      ActionAdded,
      ActionRemoved
    };

    Type m_Type;
    const WActionDescriptor* m_pDesc;
    WActionDescriptorHandle m_Handle;
  };

  static WEvent<const Event&> s_Events;

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionManager);

  static void Startup();
  static void Shutdown();
  static WActionDescriptor* CreateActionDesc(const WActionDescriptor& desc);
  static void DeleteActionDesc(WActionDescriptor* pDesc);

  struct CategoryData
  {
    WSet<WActionDescriptorHandle> m_Actions;
    WHashTable<WStringView, WActionDescriptorHandle> m_ActionNameToHandle;
  };

private:
  static WIdTable<WActionId, WActionDescriptor*> s_ActionTable;
  static WMap<WString, CategoryData> s_CategoryPathToActions;
  static WMap<WString, WString> s_ShortcutOverride;
};

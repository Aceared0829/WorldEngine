#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QKeySequence>
#include <ToolsFoundation/Document/DocumentManager.h>

class QWidget;
struct WActionDescriptor;
class WAction;
struct WActionContext;

using WActionId = WGenericId<24, 8>;

/// Creates an instance of an action for the given context. See WActionDescriptor::CreateAction().
using CreateActionFunc = WAction* (*)(const WActionContext&);
/// Destroys an instance created by a CreateActionFunc. If none is given, the action is deleted with the default allocator.
using DeleteActionFunc = void (*)(WAction*);

/// Handle for a WActionDescriptor.
///
/// WAction can be invalidated at runtime so don't store them.
class W_GUIFOUNDATION_DLL WActionDescriptorHandle
{
public:
  using StorageType = WUInt32;

  W_DECLARE_HANDLE_TYPE(WActionDescriptorHandle, WActionId);
  friend class WActionManager;

public:
  const WActionDescriptor* GetDescriptor() const;
};

/// Determines the range in which an action's shortcut is active and which contexts it needs.
struct WActionScope
{
  enum Enum
  {
    Global,   ///< Available application wide, independent of any document or window.
    Document, ///< Requires WActionContext::m_pDocument. Its shortcut is only active while that document's window has focus.
    Window,   ///< Requires WActionContext::m_pWindow. Its shortcut is only active within that window.
    Default = Global
  };
  using StorageType = WUInt8;
};

/// What kind of UI element an action maps to when a menu, menu bar or toolbar is built from an action map.
struct WActionType
{
  enum Enum
  {
    Action,        ///< A single clickable item (menu entry, toolbar button).
    Category,      ///< Groups the items mapped below it, displayed as a separator or a separate toolbar section.
    Menu,          ///< A sub-menu that only holds other items, it cannot be executed itself.
    ActionAndMenu, ///< Can both be executed and opened as a sub-menu, e.g. a toolbar button with an attached drop-down.
    Default = Action
  };
  using StorageType = WUInt8;
};

/// The environment that an action instance operates on.
///
/// Which members have to be filled out depends on the WActionScope of the action.
struct W_GUIFOUNDATION_DLL WActionContext
{
  WActionContext() = default;
  WActionContext(WDocument* pDoc) { m_pDocument = pDoc; }

  WDocument* m_pDocument = nullptr; ///< The document that the action shall affect. Required for WActionScope::Document.
  WString m_sMapping;               ///< Name of the WActionMap from which the UI element was built.
  QWidget* m_pWindow = nullptr;      ///< The widget that the action belongs to. Required for WActionScope::Window.
};


/// Describes a type of action, from which any number of action instances can be created.
///
/// Descriptors are registered once (see WActionManager and the W_REGISTER_ACTION macros) and hold everything
/// that is shared between all instances, such as the name and the configured shortcut. Refer to them through
/// WActionDescriptorHandle rather than by pointer.
struct W_GUIFOUNDATION_DLL WActionDescriptor
{
  WActionDescriptor() = default;
  ;
  WActionDescriptor(WActionType::Enum type, WActionScope::Enum scope, const char* szName, const char* szCategoryPath, const char* szShortcut,
    CreateActionFunc createAction, DeleteActionFunc deleteAction = nullptr);

  WActionDescriptorHandle m_Handle; ///< Set by WActionManager during registration.
  WEnum<WActionType> m_Type;

  WEnum<WActionScope> m_Scope;
  WString m_sActionName;   ///< Unique within category path, shown in key configuration dialog
  WString m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"

  WString m_sShortcut;     ///< The currently configured shortcut. May be modified by the user, empty means no shortcut.
  WString m_sDefaultShortcut; ///< The shortcut that the action was registered with, used to reset m_sShortcut.

  /// Creates an action instance for the given context and adds it to GetCreatedActions().
  ///
  /// The result must be destroyed through DeleteAction(), not deleted directly. Usually only called by the
  /// view classes that build menus and toolbars from an action map.
  WAction* CreateAction(const WActionContext& context) const;

  /// Destroys an instance that was returned by CreateAction().
  void DeleteAction(WAction* pAction) const;

  /// Makes all existing instances broadcast their status update event, e.g. after the shortcut was reconfigured.
  void UpdateExistingActions();

  /// The action instances that currently exist for this descriptor.
  ///
  /// One descriptor can have any number of live instances, because the same action may be mapped into
  /// several windows, menus and toolbars at once, each with its own context. State such as
  /// WButtonAction::IsEnabled() lives on these instances, not on the descriptor, so this is the only
  /// way to observe an action's current state without creating an instance.
  WArrayPtr<WAction* const> GetCreatedActions() const { return m_CreatedActions; }

private:
  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;

  mutable WHybridArray<WAction*, 4> m_CreatedActions;
};



/// Base class for all actions, meaning commands that can be triggered through menus, toolbars or shortcuts.
///
/// An instance is always tied to one WActionContext and is created through its WActionDescriptor.
/// Derived classes are typically not instantiated directly, see WButtonAction, WCategoryAction, WMenuAction and others.
class W_GUIFOUNDATION_DLL WAction : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAction, WReflectedClass);
  W_DISALLOW_COPY_AND_ASSIGN(WAction);

public:
  WAction(const WActionContext& context) { m_Context = context; }

  /// Performs whatever the action does.
  ///
  /// The meaning of the value depends on the concrete action, for buttons it is typically the new checked state,
  /// for others it is an invalid variant.
  virtual void Execute(const WVariant& value) = 0;

  /// Recomputes the action's enabled/visible state.
  ///
  /// Action proxies are cached and reused, so an action whose state depends on something outside its
  /// context (such as the asset browser selection) cannot compute it once in its constructor.
  /// Callers that build a menu from such actions have to call this before showing it.
  virtual void RefreshState() {}

  /// Broadcasts m_StatusUpdateEvent, so that the UI element displaying this action updates itself.
  void TriggerUpdate();

  const WActionContext& GetContext() const { return m_Context; }
  WActionDescriptorHandle GetDescriptorHandle() { return m_hDescriptorHandle; }

public:
  WEvent<WAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)

protected:
  WActionContext m_Context;

private:
  friend struct WActionDescriptor;
  WActionDescriptorHandle m_hDescriptorHandle;
};

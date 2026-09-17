#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Manages window layout save/restore using ADS perspective management.
class W_EDITORFRAMEWORK_DLL WWindowLayoutActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();
  static void MapActions(WStringView sMapping);

  /// Automatically restores the default layout at startup.
  static void RestoreUserLayout();

  /// Automatically saves the current layout as default at shutdown.
  static void SaveUserLayout();

  static WActionDescriptorHandle s_hCatWindowLayout;
  static WActionDescriptorHandle s_hSetToDefaultPinned;
  static WActionDescriptorHandle s_hSetToDefaultUnpinned;
  static WActionDescriptorHandle s_hSetToAbBottom;
  static WActionDescriptorHandle s_hSaveLayout;
  static WActionDescriptorHandle s_hLoadLayout;
};

class W_EDITORFRAMEWORK_DLL WWindowLayoutAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WWindowLayoutAction, WButtonAction);

public:
  enum class ButtonType
  {
    SetToDefaultPinned,
    SetToDefaultUnpinned,
    SetToAbBottom,
  };

  WWindowLayoutAction(const WActionContext& context, const char* szName, ButtonType button);
  virtual void Execute(const WVariant& value) override;

private:
  ButtonType m_ButtonType;
};

/// Dynamic menu listing the 3 user layout slots for saving the current panel arrangement.
class W_EDITORFRAMEWORK_DLL WSaveLayoutMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WSaveLayoutMenuAction, WDynamicMenuAction);

public:
  WSaveLayoutMenuAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

/// Dynamic menu listing the 3 user layout slots for restoring a previously saved panel arrangement.
class W_EDITORFRAMEWORK_DLL WLoadLayoutMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WLoadLayoutMenuAction, WDynamicMenuAction);

public:
  WLoadLayoutMenuAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);
  static void MapContextMenuActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hPickContextScene;
  static WActionDescriptorHandle s_hPickContextObject;
  static WActionDescriptorHandle s_hClearContextObject;
};

class W_EDITORFRAMEWORK_DLL WGameObjectContextAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectContextAction, WButtonAction);

public:
  enum class ActionType
  {
    PickContextScene,
    PickContextObject,
    ClearContextObject,
  };

  WGameObjectContextAction(const WActionContext& context, const char* szName, ActionType type);
  ~WGameObjectContextAction();

  virtual void Execute(const WVariant& value) override;

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void Update();

  ActionType m_Type;
};

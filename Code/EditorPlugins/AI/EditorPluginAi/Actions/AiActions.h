#pragma once

#include <EditorPluginAi/EditorPluginAiDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class W_EDITORPLUGINAI_DLL WAiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static WActionDescriptorHandle s_hCategoryAi;
  static WActionDescriptorHandle s_hProjectSettings;
};

class W_EDITORPLUGINAI_DLL WAiAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WAiAction, WButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  WAiAction(const WActionContext& context, const char* szName, ActionType type);
  ~WAiAction();

  virtual void Execute(const WVariant& value) override;

private:
  ActionType m_Type;
};

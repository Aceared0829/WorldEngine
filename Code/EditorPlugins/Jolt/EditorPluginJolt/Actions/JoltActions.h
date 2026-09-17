#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class W_EDITORPLUGINJOLT_DLL WJoltActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static WActionDescriptorHandle s_hCategoryJolt;
  static WActionDescriptorHandle s_hProjectSettings;
};

class W_EDITORPLUGINJOLT_DLL WJoltAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WJoltAction, WButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  WJoltAction(const WActionContext& context, const char* szName, ActionType type);
  ~WJoltAction();

  virtual void Execute(const WVariant& value) override;

private:
  ActionType m_Type;
};

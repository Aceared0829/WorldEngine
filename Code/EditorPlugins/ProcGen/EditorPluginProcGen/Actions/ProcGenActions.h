#pragma once

#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WPreferences;

class W_EDITORPLUGINPROCGEN_DLL WProcGenActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hDumpAST;
  static WActionDescriptorHandle s_hDumpDisassembly;
};

class W_EDITORPLUGINPROCGEN_DLL WProcGenAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenAction, WButtonAction);

public:
  enum class ActionType
  {
    DumpAST,
    DumpDisassembly,
  };

  WProcGenAction(const WActionContext& context, const char* szName, ActionType type);
  ~WProcGenAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);

  ActionType m_Type;
};

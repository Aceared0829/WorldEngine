#pragma once

#include <EditorPluginAngelScript/EditorPluginAngelScriptDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WAngelScriptAssetDocument;
struct WAngelScriptAssetEvent;

class WAngelScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActionsMenu(WStringView sMapping);
  static void MapActionsToolbar(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hOpenInVSC;
  static WActionDescriptorHandle s_hSyncExposedParams;
};

class WAngelScriptAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptAction, WButtonAction);

public:
  enum class ActionType
  {
    OpenInVSC,
    SyncExposedParameters,
  };

  WAngelScriptAction(const WActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const WVariant& value) override;

private:
  WAngelScriptAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};

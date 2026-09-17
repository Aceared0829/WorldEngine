#include <EditorPluginAngelScript/EditorPluginAngelScriptPCH.h>

#include <EditorPluginAngelScript/Actions/AngelScriptActions.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WAngelScriptActions::s_hCategory;
WActionDescriptorHandle WAngelScriptActions::s_hOpenInVSC;
WActionDescriptorHandle WAngelScriptActions::s_hSyncExposedParams;

void WAngelScriptActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("AngelScriptCategory");
  s_hOpenInVSC = W_REGISTER_ACTION_1("AngelScript.OpenInVSC", WActionScope::Document, "AngelScript", "", WAngelScriptAction, WAngelScriptAction::ActionType::OpenInVSC);
  s_hSyncExposedParams = W_REGISTER_ACTION_1("AngelScript.SyncExposedParams", WActionScope::Document, "AngelScript", "", WAngelScriptAction, WAngelScriptAction::ActionType::SyncExposedParameters);
}

void WAngelScriptActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hOpenInVSC);
  WActionManager::UnregisterAction(s_hSyncExposedParams);
}

void WAngelScriptActions::MapActionsMenu(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "G.Asset", 5.0f);

  pMap->MapAction(s_hOpenInVSC, "AngelScriptCategory", 1.0f);
  pMap->MapAction(s_hSyncExposedParams, "AngelScriptCategory", 2.0f);
}

void WAngelScriptActions::MapActionsToolbar(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "AngelScriptCategory";

  pMap->MapAction(s_hOpenInVSC, szSubPath, 1.0f);
  pMap->MapAction(s_hSyncExposedParams, szSubPath, 2.0f);
}

WAngelScriptAction::WAngelScriptAction(const WActionContext& context, const char* szName, WAngelScriptAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<WAngelScriptAssetDocument*>(static_cast<const WAngelScriptAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
    case ActionType::SyncExposedParameters:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources.svg");
      break;
  }
}


void WAngelScriptAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      m_pDocument->OpenExternalEditor();
      return;
    case ActionType::SyncExposedParameters:
      m_pDocument->SyncExposedParameters();
      return;
  }
}

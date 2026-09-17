#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WRmlUiActions::s_hCategory;
WActionDescriptorHandle WRmlUiActions::s_hOpenInVSC;

void WRmlUiActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("RmlUiCategory");
  s_hOpenInVSC = W_REGISTER_ACTION_1("RmlUi.OpenInVSC", WActionScope::Document, "RmlUi", "", WRmlUiAction, WRmlUiAction::ActionType::OpenInVSC);
}

void WRmlUiActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hOpenInVSC);
}

void WRmlUiActions::MapActionsMenu(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "G.Asset", 5.0f);

  pMap->MapAction(s_hOpenInVSC, "RmlUiCategory", 1.0f);
}

void WRmlUiActions::MapActionsToolbar(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "RmlUiCategory";

  pMap->MapAction(s_hOpenInVSC, szSubPath, 1.0f);
}

WRmlUiAction::WRmlUiAction(const WActionContext& context, const char* szName, WRmlUiAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<WRmlUiAssetDocument*>(static_cast<const WRmlUiAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
  }
}


void WRmlUiAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      m_pDocument->OpenExternalEditor();
      return;
  }
}

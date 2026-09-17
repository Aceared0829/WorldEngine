#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/Actions/AiActions.h>
#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAiAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WAiActions::s_hCategoryAi;
WActionDescriptorHandle WAiActions::s_hProjectSettings;

void WAiActions::RegisterActions()
{
  s_hCategoryAi = W_REGISTER_CATEGORY("Ai");
  s_hProjectSettings = W_REGISTER_ACTION_1("Ai.Settings.Project", WActionScope::Document, "Ai", "", WAiAction, WAiAction::ActionType::ProjectSettings);
}

void WAiActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategoryAi);
  WActionManager::UnregisterAction(s_hProjectSettings);
}

void WAiActions::MapMenuActions()
{
  WActionMap* pMap = WActionMapManager::GetActionMap("AssetMenuBar");
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAi, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Ai", 1.0f);
}

WAiAction::WAiAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AiPlugin/WAiPlugin.svg");
      break;
  }
}

WAiAction::~WAiAction() = default;

void WAiAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    WQtAiProjectSettingsDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Accepted)
    {
      WToolsProject::BroadcastConfigChanged();
    }
  }
}

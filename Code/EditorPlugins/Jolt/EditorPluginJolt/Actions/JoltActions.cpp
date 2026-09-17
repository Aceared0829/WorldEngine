#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WJoltActions::s_hCategoryJolt;
WActionDescriptorHandle WJoltActions::s_hProjectSettings;

void WJoltActions::RegisterActions()
{
  s_hCategoryJolt = W_REGISTER_CATEGORY("Jolt");
  s_hProjectSettings = W_REGISTER_ACTION_1("Jolt.Settings.Project", WActionScope::Document, "Jolt", "", WJoltAction, WJoltAction::ActionType::ProjectSettings);
}

void WJoltActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategoryJolt);
  WActionManager::UnregisterAction(s_hProjectSettings);
}

void WJoltActions::MapMenuActions()
{
  WActionMap* pMap = WActionMapManager::GetActionMap("AssetMenuBar");
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryJolt, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Jolt", 1.0f);
}

WJoltAction::WJoltAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/JoltPlugin/JoltPlugin.svg");
      break;
  }
}

WJoltAction::~WJoltAction() = default;

void WJoltAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    WQtJoltProjectSettingsDlg dlg(value);
    if (dlg.exec() == QDialog::Accepted)
    {
      WToolsProject::BroadcastConfigChanged();
    }
  }
}

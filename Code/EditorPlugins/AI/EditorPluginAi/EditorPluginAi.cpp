#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginAi/Actions/AiActions.h>
#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnLoadPlugin()
{
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  WAiActions::RegisterActions();
  WAiActions::MapMenuActions();
}

void OnUnloadPlugin()
{
  WAiActions::UnregisterActions();
  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateGroundTypeDynamicEnumValues()
{
  WAiNavigationConfig cfg;
  cfg.Load().IgnoreResult();

  {
    auto& cfe = WDynamicEnum::GetDynamicEnum("AiGroundType");
    cfe.Clear();

    cfe.SetValueAndName(-1, "<Undefined>");

    // add all names and values that are active
    for (WInt32 i = 0; i < WAiNumGroundTypes; ++i)
    {
      if (cfg.m_GroundTypes[i].m_bUsed)
      {
        cfe.SetValueAndName(i, cfg.m_GroundTypes[i].m_sName);
      }
    }
  }

  {
    auto& de = WDynamicStringEnum::CreateDynamicEnum("AiPathSearchConfig");
    de.Clear();

    for (const auto& pc : cfg.m_PathSearchConfigs)
    {
      de.AddValidValue(pc.m_sName);
    }

    de.SortValues();
  }

  {
    auto& de = WDynamicStringEnum::CreateDynamicEnum("AiNavmeshConfig");
    de.Clear();

    for (const auto& pc : cfg.m_NavmeshConfigs)
    {
      de.AddValidValue(pc.m_sName);
    }

    de.SortValues();
  }
}

static void ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  if (e.m_Type == WToolsProjectEvent::Type::ProjectSaveState)
  {
  }

  if (e.m_Type == WToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateGroundTypeDynamicEnumValues();
  }
}

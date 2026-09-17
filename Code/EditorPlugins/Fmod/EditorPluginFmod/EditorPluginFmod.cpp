#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnLoadPlugin()
{
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Mesh
  {
    // Menu Bar
    WActionMapManager::RegisterActionMap("SoundBankAssetMenuBar", "AssetMenuBar");

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("SoundBankAssetToolBar", "AssetToolbar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      WFmodActions::RegisterActions();
      WFmodActions::MapPluginMenuActions("AssetMenuBar");
      WFmodActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      WFmodActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      WFmodActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      WFmodActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  WFmodActions::UnregisterActions();
  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

static void ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  if (e.m_Type == WToolsProjectEvent::Type::ProjectOpened)
  {
    WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();
    pPreferences->SyncCVars();
  }
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

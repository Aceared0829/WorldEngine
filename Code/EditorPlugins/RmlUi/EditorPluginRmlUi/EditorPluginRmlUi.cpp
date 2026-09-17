#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    WRmlUiActions::RegisterActions();

    // Menu Bar
    {
      WActionMapManager::RegisterActionMap("RmlUiAssetMenuBar", "AssetMenuBar");
      WRmlUiActions::MapActionsMenu("RmlUiAssetMenuBar");
    }

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("RmlUiAssetToolBar", "AssetToolbar");
      WRmlUiActions::MapActionsToolbar("RmlUiAssetToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  WRmlUiActions::UnregisterActions();
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

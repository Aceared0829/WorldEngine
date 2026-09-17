#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // VisualScript
  {
    // Menu Bar
    {
      WActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar", "AssetMenuBar");
      WEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("VisualScriptAssetToolBar", "AssetToolbar");
    }
  }
}

void OnUnloadPlugin()
{
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

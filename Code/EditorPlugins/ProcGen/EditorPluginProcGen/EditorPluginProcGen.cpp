#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnLoadPlugin()
{
  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "ProcGenAssetMenuBar";
      WActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
      WEditActions::MapActions(szMenuBar, false, false);
    }

    // Tool Bar
    {
      const char* szToolBar = "ProcGenAssetToolBar";
      WActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      WProcGenActions::RegisterActions();
      WProcGenActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  WProcGenActions::UnregisterActions();
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

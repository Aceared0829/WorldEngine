#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnLoadPlugin()
{
  WSubstancePackageAssetActions::RegisterActions();

  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "SubstanceAssetMenuBar";

      WActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
      WEditActions::MapActions(szMenuBar, false, false);
    }

    // Tool Bar
    {
      const char* szToolBar = "SubstanceAssetToolBar";
      WActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");
      WSubstancePackageAssetActions::MapToolbarActions("SubstanceAssetToolBar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  WSubstancePackageAssetActions::UnregisterActions();
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

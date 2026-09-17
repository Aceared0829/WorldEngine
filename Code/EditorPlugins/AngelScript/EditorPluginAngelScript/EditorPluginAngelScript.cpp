#include <EditorPluginAngelScript/EditorPluginAngelScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginAngelScript/Actions/AngelScriptActions.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void OnLoadPlugin()
{
  WAngelScriptActions::RegisterActions();

  // AngelScript
  {
    // Menu Bar
    {
      WActionMapManager::RegisterActionMap("AngelScriptAssetMenuBar", "AssetMenuBar");

      WEditActions::MapActions("AngelScriptAssetMenuBar", false, false);
      WAngelScriptActions::MapActionsMenu("AngelScriptAssetMenuBar");
    }

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("AngelScriptAssetToolBar", "AssetToolbar");
      WAngelScriptActions::MapActionsToolbar("AngelScriptAssetToolBar");
    }

    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WAngelScriptAssetDocument::PropertyMetaStateEventHandler);
  }
}

void OnUnloadPlugin()
{
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WAngelScriptAssetDocument::PropertyMetaStateEventHandler);

  WAngelScriptActions::UnregisterActions();
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

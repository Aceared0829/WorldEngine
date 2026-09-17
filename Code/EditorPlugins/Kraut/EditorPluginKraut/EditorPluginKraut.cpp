#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

W_PLUGIN_ON_LOADED()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WKrautTreeAssetProperties::PropertyMetaStateEventHandler);

  WKrautActions::RegisterActions();

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("KrautTreeAssetToolBar", "AssetToolbar");
    WKrautActions::MapActions("KrautTreeAssetToolBar");
  }
}

W_PLUGIN_ON_UNLOADED()
{
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WKrautTreeAssetProperties::PropertyMetaStateEventHandler);
  WKrautActions::UnregisterActions();
}

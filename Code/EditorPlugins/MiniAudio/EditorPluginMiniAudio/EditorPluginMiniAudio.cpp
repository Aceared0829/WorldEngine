#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginMiniAudio/Actions/MiniAudioActions.h>
#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnLoadPlugin()
{
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler);

  // Mesh
  {
    // Menu Bar
    WActionMapManager::RegisterActionMap("MiniAudioSoundAssetMenuBar", "AssetMenuBar");

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("MiniAudioSoundAssetToolBar", "AssetToolbar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      WMiniAudioActions::RegisterActions();
      WMiniAudioActions::MapPluginMenuActions("AssetMenuBar");
      WMiniAudioActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      WMiniAudioActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      WMiniAudioActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      WMiniAudioActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  WMiniAudioActions::UnregisterActions();
  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler);
}

static void ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  if (e.m_Type == WToolsProjectEvent::Type::ProjectOpened)
  {
    WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();
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

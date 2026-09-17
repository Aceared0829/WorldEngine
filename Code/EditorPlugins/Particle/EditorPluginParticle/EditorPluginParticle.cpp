#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void OnLoadPlugin()
{
  WParticleActions::RegisterActions();

  // Particle Effect
  {
    // Menu Bar
    {
      WActionMapManager::RegisterActionMap("ParticleEffectAssetMenuBar", "AssetMenuBar");
    }

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("ParticleEffectAssetToolBar", "AssetToolbar");
      WParticleActions::MapActions("ParticleEffectAssetToolBar");
    }

    // View Tool Bar
    {
      WActionMapManager::RegisterActionMap("ParticleEffectAssetViewToolBar", "SimpleAssetViewToolbar");
    }

    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WParticleEffectAssetDocument::PropertyMetaStateEventHandler);
  }
}

void OnUnloadPlugin()
{
  WParticleActions::UnregisterActions();
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WParticleEffectAssetDocument::PropertyMetaStateEventHandler);
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

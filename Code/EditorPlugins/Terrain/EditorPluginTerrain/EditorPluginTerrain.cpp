#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush2DVisualizerAdapter.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush3DVisualizerAdapter.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>

static void WTerrainPatchComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WTerrainPatchComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const WString sHeightImage = e.m_pObject->GetTypeAccessor().GetValue("HeightImage").ConvertTo<WString>();
  const bool bHasHeightImage = !sHeightImage.IsEmpty();

  auto& props = *e.m_pPropertyStates;
  props["HeightImageOffset"].m_Visibility = bHasHeightImage ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["HeightImageSize"].m_Visibility = bHasHeightImage ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["HeightImageScale"].m_Visibility = bHasHeightImage ? WPropertyUiState::Default : WPropertyUiState::Invisible;
}

W_PLUGIN_ON_LOADED()
{
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WTerrainBrush2DVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WTerrainBrush2DVisualizerAdapter); });
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WTerrainBrush3DVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WTerrainBrush3DVisualizerAdapter); });
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WTerrainPatchComponent_PropertyMetaStateEventHandler);
}

W_PLUGIN_ON_UNLOADED()
{
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WTerrainBrush2DVisualizerAttribute>());
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WTerrainBrush3DVisualizerAttribute>());
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WTerrainPatchComponent_PropertyMetaStateEventHandler);
}

#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

W_STATICLINK_LIBRARY(EnginePluginTerrain)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(EnginePluginTerrain_SceneExport_TerrainHeightfieldExportModifier);
  W_STATICLINK_REFERENCE(EnginePluginTerrain_SceneExport_TerrainVoxelExportModifier);
}

#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/SceneExport/JoltFinalizeGeneratedCollision.h>
#include <JoltPlugin/Components/JoltGenerateCollisionComponent.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_JoltFinalizeGeneratedCollision, 1, WRTTIDefaultAllocator<WSceneExportModifier_JoltFinalizeGeneratedCollision>)
W_END_DYNAMIC_REFLECTED_TYPE;

void WSceneExportModifier_JoltFinalizeGeneratedCollision::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  // Don't finalize yet for prefabs since the real generation might only happen in the final scene context
  if (sDocumentType == "Prefab" && bForExport)
  {
    return;
  }

  W_LOCK(ref_world.GetWriteMarker());

  auto pComponentManager = ref_world.GetComponentManager<WJoltGenerateCollisionComponentManager>();
  if (pComponentManager == nullptr)
    return;

  for (auto it = pComponentManager->GetComponents(); it.IsValid(); ++it)
  {
    it->FinalizeGeneration();

    pComponentManager->DeleteComponent(it);
  }
}

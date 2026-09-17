#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSceneExportModifier::CreateModifiers(WDynamicArray<WSceneExportModifier*>& ref_modifiers)
{
  WRTTI::ForEachDerivedType<WSceneExportModifier>(
    [&](const WRTTI* pRtti)
    {
      WSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<WSceneExportModifier>();
      ref_modifiers.PushBack(pMod);
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void WSceneExportModifier::DestroyModifiers(WDynamicArray<WSceneExportModifier*>& ref_modifiers)
{
  for (auto pMod : ref_modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  ref_modifiers.Clear();
}

void WSceneExportModifier::ApplyAllModifiers(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  WTempHybridArray<WSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(ref_world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(ref_world);
}

void VisitObject(WWorld& ref_world, WGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(ref_world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!pObject->GetName().IsEmpty())
    return;

  if (!pObject->GetGlobalKey().IsEmpty())
    return;

  ref_world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void WSceneExportModifier::CleanUpWorld(WWorld& ref_world)
{
  W_LOCK(ref_world.GetWriteMarker());

  // Don't do this (for now), as we would also delete objects that are referenced by other components,
  // and currently we can't know which ones are important to keep.

  // for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  //{
  //   // only visit objects without parents, those are the root objects
  //   if (it->GetParent() != nullptr)
  //     continue;

  //  VisitObject(world, it);
  //}

  const bool bSim = ref_world.GetWorldSimulationEnabled();
  ref_world.SetWorldSimulationEnabled(false);
  ref_world.Update();
  ref_world.SetWorldSimulationEnabled(bSim);
}

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <EnginePluginScene/SceneExport/ExportModifiers.h>
#include <GameEngine/Messages/ExportMessage.h>
#include <RendererCore/Components/SplineComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_RemoveShapeIconComponents, 1, WRTTIDefaultAllocator<WSceneExportModifier_RemoveShapeIconComponents>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  W_LOCK(ref_world.GetWriteMarker());

  if (WShapeIconComponentManager* pSiMan = ref_world.GetComponentManager<WShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_RemovePathNodeComponents, 1, WRTTIDefaultAllocator<WSceneExportModifier_RemovePathNodeComponents>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSceneExportModifier_RemovePathNodeComponents::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  W_LOCK(ref_world.GetWriteMarker());

  if (WSplineNodeComponentManager* pManager = ref_world.GetComponentManager<WSplineNodeComponentManager>())
  {
    for (auto it = pManager->GetComponents(); it.IsValid(); it.Next())
    {
      if (it->GetOwner()->GetComponents().GetCount() == 1 && it->GetOwner()->GetChildCount() == 0)
      {
        // if this is the only component on the object, clear it's name, so that the entire object may get cleaned up
        it->GetOwner()->SetName(WStringView());
      }

      pManager->DeleteComponent(it);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_GenericExport, 1, WRTTIDefaultAllocator<WSceneExportModifier_GenericExport>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSceneExportModifier_GenericExport::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  WStringBuilder sb;
  WConversionUtils::ToString(documentGuid, sb);

  W_LOCK(ref_world.GetWriteMarker());

  WMsgExport msg;
  msg.m_sDocumentType = sDocumentType;
  msg.m_sDocumentGuid = sb;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }
}

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/DecalDragDropHandler.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalComponentDragDropHandler, 1, WRTTIDefaultAllocator<WDecalComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;


float WDecalComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Decal") ? 1.0f : 0.0f;
}

void WDecalComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  WVariantArray var;
  var.PushBack(GetAssetGuidString(pInfo));
  constexpr const char* szComponentType = "WDecalComponent";
  constexpr const char* szPropertyName = "Decals";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, var, pInfo->m_ActiveParentObject, -1);

    m_vAlignAxisWithNormal = -WVec3::MakeAxisX();
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, var, pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, var, pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshComponentDragDropHandler, 1, WRTTIDefaultAllocator<WMeshComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;

float WMeshComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Mesh") ? 1.0f : 0.0f;
}

void WMeshComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "WMeshComponent";
  constexpr const char* szPropertyName = "Mesh";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshComponentDragDropHandler, 1, WRTTIDefaultAllocator<WAnimatedMeshComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;

float WAnimatedMeshComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Animated Mesh") ? 1.0f : 0.0f;
}

void WAnimatedMeshComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "WAnimatedMeshComponent";
  constexpr const char* szPropertyName = "Mesh";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

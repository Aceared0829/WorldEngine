#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginKraut/DragDropHandlers/KrautTreeDragDropHandler.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WKrautTreeComponentDragDropHandler, 1, WRTTIDefaultAllocator<WKrautTreeComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;


float WKrautTreeComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Kraut Tree") ? 1.0f : 0.0f;
}

void WKrautTreeComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "WKrautTreeComponent";
  constexpr const char* szPropertyName = "KrautTree";

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

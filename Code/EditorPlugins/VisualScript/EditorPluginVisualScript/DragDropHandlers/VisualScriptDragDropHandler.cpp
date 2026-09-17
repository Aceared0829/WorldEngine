#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginVisualScript/DragDropHandlers/VisualScriptDragDropHandler.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptComponentDragDropHandler, 1, WRTTIDefaultAllocator<WVisualScriptComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;


float WVisualScriptComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "VisualScriptClass") ? 1.0f : 0.0f;
}

void WVisualScriptComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "WScriptComponent";
  constexpr const char* szPropertyName = "ScriptClass";

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
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

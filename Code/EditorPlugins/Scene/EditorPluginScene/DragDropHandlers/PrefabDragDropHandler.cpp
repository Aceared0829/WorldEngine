#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/PrefabDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabCache.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPrefabComponentDragDropHandler, 1, WRTTIDefaultAllocator<WPrefabComponentDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;


float WPrefabComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (WComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Prefab") ? 1.0f : 0.0f;
}

void WPrefabComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_bShiftKeyDown)
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_ActiveParentObject, -1);
    else
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }
  else
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreateDropObject(pInfo->m_vDropPosition, "WPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
    else
      CreateDropObject(pInfo->m_vDropPosition, "WPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

void WPrefabComponentDragDropHandler::CreatePrefab(const WVec3& vPosition, const WUuid& AssetGuid, WUuid parent, WInt32 iInsertChildIndex)
{
  WVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  auto pCmdHistory = m_pDocument->GetCommandHistory();

  WInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_Parent = parent;
  PasteCmd.m_CreateFromPrefab = AssetGuid;
  PasteCmd.m_Index = iInsertChildIndex;
  PasteCmd.m_sBasePrefabGraph = WPrefabCache::GetSingleton()->GetCachedPrefabDocument(AssetGuid);
  PasteCmd.m_RemapGuid = WUuid::MakeUuid();

  if (PasteCmd.m_sBasePrefabGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd).AssertSuccess();

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPos, WQuat::MakeIdentity());

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}

void WPrefabComponentDragDropHandler::OnDragUpdate(const WDragDropInfo* pInfo)
{
  WComponentDragDropHandler::OnDragUpdate(pInfo);

  // the way prefabs are instantiated on the runtime side means the selection is not always immediately 'correct'
  // by resetting the selection, we can fix this
  SelectCreatedObjects();
}

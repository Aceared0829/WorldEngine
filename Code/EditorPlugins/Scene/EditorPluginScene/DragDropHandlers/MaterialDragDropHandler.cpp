#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialDragDropHandler, 1, WRTTIDefaultAllocator<WMaterialDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;

void WMaterialDragDropHandler::RequestConfiguration(WDragDropConfig* pConfigToFillOut)
{
  pConfigToFillOut->m_bPickSelectedObjects = true;
}

float WMaterialDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport")
    return 0.0f;

  const WDocument* pDocument = WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom<WSceneDocument>())
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Material") ? 1.0f : 0.0f;
}

void WMaterialDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  m_pDocument = WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  W_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
}

void WMaterialDragDropHandler::OnDragUpdate(const WDragDropInfo* pInfo)
{
  if (!pInfo->m_TargetComponent.IsValid())
    return;

  const WDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

  if (!pComponent)
    return;

  if (m_AppliedToComponent == pInfo->m_TargetComponent && m_iAppliedToSlot == pInfo->m_iTargetObjectSubID)
    return;

  m_AppliedToComponent = pInfo->m_TargetComponent;
  m_iAppliedToSlot = pInfo->m_iTargetObjectSubID;

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WMeshComponentBase>())
  {
    WResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_Index = pInfo->m_iTargetObjectSubID;
    cmd.m_sProperty = "Materials";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WGreyBoxComponent>())
  {
    WSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_sProperty = "Material";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }
}

void WMaterialDragDropHandler::OnDragCancel()
{
  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void WMaterialDragDropHandler::OnDrop(const WDragDropInfo* pInfo)
{
  if (pInfo->m_TargetComponent.IsValid())
  {
    const WDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

    if (pComponent && (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WMeshComponent>() ||
                        pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WGreyBoxComponent>()))
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
      return;
    }
  }

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

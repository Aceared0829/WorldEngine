#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "EditorFramework/GUI/RawDocumentTreeModel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/LayerDragDropHandler.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QIODevice>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerDragDropHandler, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

const WRTTI* WLayerDragDropHandler::GetCommonBaseType(const WDragDropInfo* pInfo) const
{
  QByteArray encodedData = pInfo->m_pMimeData->data("application/WEditor.ObjectSelection");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  WTempHybridArray<WDocumentObject*, 32> Dragged;
  stream >> Dragged;

  const WRTTI* pCommonBaseType = nullptr;
  for (const WDocumentObject* pItem : Dragged)
  {
    pCommonBaseType = pCommonBaseType == nullptr ? pItem->GetType() : WReflectionUtils::GetCommonBaseType(pCommonBaseType, pItem->GetType());
  }
  return pCommonBaseType;
}


//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerOnLayerDragDropHandler, 1, WRTTIDefaultAllocator<WLayerOnLayerDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;

float WLayerOnLayerDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/WEditor.ObjectSelection"))
  {
    if (WScene2Document* pDoc = WDynamicCast<WScene2Document*>(WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const WDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pInfo->m_pAdapter && GetCommonBaseType(pInfo)->IsDerivedFrom(WGetStaticRTTI<WSceneLayerBase>()))
      {
        const WAbstractProperty* pTargetProp = pInfo->m_pAdapter->GetType()->FindPropertyByName(pInfo->m_pAdapter->GetChildProperty());
        if (pTargetProp && WGetStaticRTTI<WSceneLayerBase>()->IsDerivedFrom(pTargetProp->GetSpecificType()))
          return 1.0f;
      }
    }
  }
  return 0;
}

void WLayerOnLayerDragDropHandler::OnDrop(const WDragDropInfo* pInfo)
{
  if (WScene2Document* pDoc = WDynamicCast<WScene2Document*>(WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const WUuid activeDoc = pDoc->GetActiveLayer();
    W_VERIFY(pDoc->SetActiveLayer(pDoc->GetGuid()).Succeeded(), "Failed to set active document.");
    {
      // We need to make a copy of the info as the target document is actually the scene here, not the active document.
      WDragDropInfo info = *pInfo;
      info.m_TargetDocument = pDoc->GetGuid();
      WQtDocumentTreeModel::MoveObjects(info);
    }
    W_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectOnLayerDragDropHandler, 1, WRTTIDefaultAllocator<WGameObjectOnLayerDragDropHandler>)
W_END_DYNAMIC_REFLECTED_TYPE;

float WGameObjectOnLayerDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/WEditor.ObjectSelection"))
  {
    if (WScene2Document* pDoc = WDynamicCast<WScene2Document*>(WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const WDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pTarget->GetType() == WGetStaticRTTI<WSceneLayer>() && pInfo->m_iTargetObjectInsertChildIndex == -1 && GetCommonBaseType(pInfo) == WGetStaticRTTI<WGameObject>())
      {
        WObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
        WUuid layerGuid = pAccessor->GetByName<WUuid>(pTarget, "Layer");
        if (pDoc->IsLayerLoaded(layerGuid))
          return 1.0f;
      }
    }
  }
  return 0;
}

void WGameObjectOnLayerDragDropHandler::OnDrop(const WDragDropInfo* pInfo)
{
  if (WScene2Document* pDoc = WDynamicCast<WScene2Document*>(WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const WDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);

    QByteArray encodedData = pInfo->m_pMimeData->data("application/WEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    WTempHybridArray<WDocumentObject*, 32> Dragged;
    stream >> Dragged;

    // We are dragging game objects on another layer => delete objects and recreate in target layer.
    WSceneDocument* pSourceDoc = WDynamicCast<WSceneDocument*>(Dragged[0]->GetDocumentObjectManager()->GetDocument());
    WObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
    WUuid layerGuid = pAccessor->GetByName<WUuid>(pTarget, "Layer");
    WSceneDocument* pTargetDoc = pDoc->GetLayerDocument(layerGuid);

    if (pSourceDoc != pTargetDoc && pTargetDoc)
    {
      const WUuid activeDoc = pDoc->GetActiveLayer();
      {
        // activeDoc should already match pSourceDoc, but just to be sure.
        W_VERIFY(pDoc->SetActiveLayer(pSourceDoc->GetGuid()).Succeeded(), "Failed to set active document.");

        WResult res = WActionManager::ExecuteAction(nullptr, "Selection.Copy", pSourceDoc, WVariant());
        if (res.Failed())
        {
          WLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
        res = WActionManager::ExecuteAction(nullptr, "Selection.Delete", pSourceDoc, WVariant());
        if (res.Failed())
        {
          WLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
      }
      {
        W_VERIFY(pDoc->SetActiveLayer(pTargetDoc->GetGuid()).Succeeded(), "Failed to set active document.");
        WResult res = WActionManager::ExecuteAction(nullptr, "Selection.PasteAtOriginalLocation", pTargetDoc, WVariant());
        if (res.Failed())
        {
          WLog::Error("Failed to paste selection while moving objects between layers.");
          return;
        }
      }
      W_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
    }
  }
}

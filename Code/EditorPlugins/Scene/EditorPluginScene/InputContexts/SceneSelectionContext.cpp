#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

WSceneSelectionContext::WSceneSelectionContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera)
  : WSelectionContext(pOwnerWindow, pOwnerView, pCamera)
{
}

void WSceneSelectionContext::OpenDocumentForPickedObject(const WObjectPickingResult& res) const
{
  WSelectionContext::OpenDocumentForPickedObject(res);
}

void WSceneSelectionContext::SelectPickedObject(const WObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  WScene2Document* pSceneDocument = nullptr;

  // If bToggle (ctrl-key) is held, we don't want to switch layers.
  // Same if we have a custom pick override set which usually means that the selection is hijacked to make an object modification on the current layer.
  if (res.m_PickedObject.IsValid() && !bToggle)
  {
    const WDocumentObject* pObject = nullptr;
    WUuid layerGuid = FindLayerByObject(res.m_PickedObject, pObject);
    if (layerGuid.IsValid())
    {
      pSceneDocument = WDynamicCast<WScene2Document*>(GetOwnerWindow()->GetDocument());
      if (pSceneDocument->IsLayerLoaded(layerGuid))
      {
        if (m_PickObjectOverride.IsValid())
        {
          m_PickObjectOverride(pObject);
          return;
        }

        if (pSceneDocument->GetActiveLayer() != layerGuid)
        {
          if (pSceneDocument->GetSwitchLayerToSelection())
          {
            pSceneDocument->PreventDoubleSelectionChange(true);
            pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
          }
          else
          {
            pSceneDocument->ShowDocumentStatus(WFmt("The clicked object is in layer '{}'. Switch layer or enable 'Auto Switch Layer to Selection'", pSceneDocument->GetLayerDocument(layerGuid)->GetDocumentPath().GetFileName()));
          }
        }
      }
    }
  }

  WSelectionContext::SelectPickedObject(res, bToggle, bDirect);

  if (pSceneDocument)
    pSceneDocument->PreventDoubleSelectionChange(false);
}

WUuid WSceneSelectionContext::FindLayerByObject(WUuid objectGuid, const WDocumentObject*& out_pObject) const
{
  WTempHybridArray<WSceneDocument*, 8> loadedLayers;
  const WScene2Document* pSceneDocument = WDynamicCast<const WScene2Document*>(GetOwnerWindow()->GetDocument());
  pSceneDocument->GetLoadedLayers(loadedLayers);
  for (WSceneDocument* pLayer : loadedLayers)
  {
    if (pLayer == pSceneDocument)
    {
      if ((out_pObject = pSceneDocument->GetSceneObjectManager()->GetObject(objectGuid)))
      {
        return pLayer->GetGuid();
      }
    }
    else if ((out_pObject = pLayer->GetObjectManager()->GetObject(objectGuid)))
    {
      return pLayer->GetGuid();
    }
  }
  out_pObject = nullptr;
  return WUuid();
}

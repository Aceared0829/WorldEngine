#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshEditorContext.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshEditorInputContext, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WMeshEditorInputContext::WMeshEditorInputContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView)
{
  SetOwner(pOwnerWindow, pOwnerView);
}

WEditorInput WMeshEditorInputContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::MiddleButton && (e->modifiers() & Qt::KeyboardModifier::ControlModifier))
  {
    const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

    auto* pMeshDoc = static_cast<WMeshAssetDocument*>(GetOwnerWindow()->GetDocument());
    auto& allSlots = pMeshDoc->GetProperties()->m_Slots;

    if (res.m_uiPartIndex < (WUInt32)allSlots.GetCount())
    {
      if (!allSlots[res.m_uiPartIndex].m_sResource.IsEmpty())
      {
        for (auto* pDocMan : WDocumentManager::GetAllDocumentManagers())
        {
          if (auto* pAssetMan = WDynamicCast<WAssetDocumentManager*>(pDocMan))
          {
            if (pAssetMan->TryOpenAssetDocument(allSlots[res.m_uiPartIndex].m_sResource).Succeeded())
              return WEditorInput::WasExclusivelyHandled;
          }
        }
        GetOwnerWindow()->ShowTemporaryStatusBarMsg("Could not open material document.");
      }
      else
      {
        GetOwnerWindow()->ShowTemporaryStatusBarMsg("No material assigned to this mesh surface.");
      }
    }
    return WEditorInput::WasExclusivelyHandled;
  }
  return WEditorInput::MayBeHandledByOthers;
}

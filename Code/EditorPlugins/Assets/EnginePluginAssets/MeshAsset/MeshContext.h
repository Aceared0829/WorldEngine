#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINASSETS_DLL WMeshContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WMeshContext, WEngineProcessDocumentContext);

public:
  WMeshContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

  /// Human-readable display name for each material slot, updated whenever materials are set.
  WHybridArray<WString, 16> m_SlotNames;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);
  void OnResourceEvent(const WResourceEvent& e);

  WGameObject* m_pMeshObject;
  WMeshResourceHandle m_hMesh;

  WAtomicBool m_bBoundsDirty = false;
  WEvent<const WResourceEvent&, WMutex>::Unsubscriber m_MeshResourceEventSubscriber;
};

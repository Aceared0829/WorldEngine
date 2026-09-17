#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINASSETS_DLL WAnimatedMeshContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WAnimatedMeshContext, WEngineProcessDocumentContext);

public:
  WAnimatedMeshContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WMeshResourceHandle& GetAnimatedMesh() const { return m_hAnimatedMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);

  WGameObject* m_pAnimatedMeshObject;
  WMeshResourceHandle m_hAnimatedMesh;
};

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINJOLT_DLL WJoltCollisionMeshContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshContext, WEngineProcessDocumentContext);

public:
  WJoltCollisionMeshContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WJoltMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);

  WGameObject* m_pMeshObject;
  WJoltMeshResourceHandle m_hMesh;
};

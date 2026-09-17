#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WJoltCollisionMeshContext;

class WJoltCollisionMeshViewContext : public WEngineProcessViewContext
{
public:
  WJoltCollisionMeshViewContext(WJoltCollisionMeshContext* pMeshContext);
  ~WJoltCollisionMeshViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WJoltCollisionMeshContext* m_pContext = nullptr;
};

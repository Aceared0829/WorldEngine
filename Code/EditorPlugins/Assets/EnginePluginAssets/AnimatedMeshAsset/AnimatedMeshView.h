#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WAnimatedMeshContext;

class WAnimatedMeshViewContext : public WEngineProcessViewContext
{
public:
  WAnimatedMeshViewContext(WAnimatedMeshContext* pMeshContext);
  ~WAnimatedMeshViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WAnimatedMeshContext* m_pContext = nullptr;
};

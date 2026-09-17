#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WAnimationClipContext;

class WAnimationClipViewContext : public WEngineProcessViewContext
{
public:
  WAnimationClipViewContext(WAnimationClipContext* pContext);
  ~WAnimationClipViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WAnimationClipContext* m_pContext = nullptr;
};

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WSkeletonContext;

class WSkeletonViewContext : public WEngineProcessViewContext
{
public:
  WSkeletonViewContext(WSkeletonContext* pContext);
  ~WSkeletonViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

  virtual void Redraw(bool bRenderEditorGizmos) override;

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  virtual void HandleViewMessage(const WEditorEngineViewMsg* pMsg) override;

  void PickObjectAt(WUInt16 x, WUInt16 y);

  WSkeletonContext* m_pContext = nullptr;
};

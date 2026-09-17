#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WKrautTreeContext;

class WKrautTreeViewContext : public WEngineProcessViewContext
{
public:
  WKrautTreeViewContext(WKrautTreeContext* pKrautTreeContext);
  ~WKrautTreeViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WKrautTreeContext* m_pKrautTreeContext;
};

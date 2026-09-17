#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WRmlUiDocumentContext;

class WRmlUiViewContext : public WEngineProcessViewContext
{
public:
  WRmlUiViewContext(WRmlUiDocumentContext* pRmlUiContext);
  ~WRmlUiViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WRmlUiDocumentContext* m_pRmlUiContext;
};

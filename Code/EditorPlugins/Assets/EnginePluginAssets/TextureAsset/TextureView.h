#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WTextureContext;

class WTextureViewContext : public WEngineProcessViewContext
{
public:
  WTextureViewContext(WTextureContext* pMaterialContext);
  ~WTextureViewContext();

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WTextureContext* m_pTextureContext;
};

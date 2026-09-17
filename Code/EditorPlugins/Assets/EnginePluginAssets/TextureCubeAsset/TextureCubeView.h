#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WTextureCubeContext;

class WTextureCubeViewContext : public WEngineProcessViewContext
{
public:
  WTextureCubeViewContext(WTextureCubeContext* pMaterialContext);
  ~WTextureCubeViewContext();

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  WTextureCubeContext* m_pTextureContext;
};

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WDecalContext;

class WDecalViewContext : public WEngineProcessViewContext
{
public:
  WDecalViewContext(WDecalContext* pDecalContext);
  ~WDecalViewContext();

protected:
  virtual WViewHandle CreateView() override;

  WDecalContext* m_pDecalContext;
};

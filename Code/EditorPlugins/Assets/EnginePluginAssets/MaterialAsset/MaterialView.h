#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WMaterialContext;

class WMaterialViewContext : public WEngineProcessViewContext
{
public:
  WMaterialViewContext(WMaterialContext* pMaterialContext);
  ~WMaterialViewContext();

  void PositionThumbnailCamera();

protected:
  virtual WViewHandle CreateView() override;

  WMaterialContext* m_pMaterialContext;
};

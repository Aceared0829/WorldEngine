#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WParticleContext;

class WParticleViewContext : public WEngineProcessViewContext
{
public:
  WParticleViewContext(WParticleContext* pParticleContext);
  ~WParticleViewContext();

  void PositionThumbnailCamera(const WBoundingBoxSphere& bounds);

protected:
  virtual WViewHandle CreateView() override;

  WParticleContext* m_pParticleContext;
};

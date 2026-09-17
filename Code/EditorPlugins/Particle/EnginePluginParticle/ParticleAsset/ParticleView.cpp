#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <RendererCore/Pipeline/View.h>

WParticleViewContext::WParticleViewContext(WParticleContext* pParticleContext)
  : WEngineProcessViewContext(pParticleContext)
{
  m_pParticleContext = pParticleContext;
}

WParticleViewContext::~WParticleViewContext() = default;

void WParticleViewContext::PositionThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);

  FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(-1.8f, 1.8f, 1.0f));
}

WViewHandle WParticleViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Particle Editor - View");
  return pView->GetHandle();
}

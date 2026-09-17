#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class WParticleComponent;

using WParticleEffectResourceHandle = WTypedResourceHandle<class WParticleEffectResource>;

class W_ENGINEPLUGINPARTICLE_DLL WParticleContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WParticleContext, WEngineProcessDocumentContext);

public:
  WParticleContext();
  ~WParticleContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual void OnThumbnailViewContextRequested() override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

private:
  WBoundingBoxSphere m_ThumbnailBoundingVolume;
  WParticleEffectResourceHandle m_hParticle;
  WMeshResourceHandle m_hPreviewMeshResource;
  WParticleComponent* m_pComponent = nullptr;
};

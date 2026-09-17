#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/Extractor.h>

class WSceneContext;
class WCameraComponent;

class WEditorSelectedObjectsExtractor : public WSelectedObjectsExtractorBase
{
  W_ADD_DYNAMIC_REFLECTION(WEditorSelectedObjectsExtractor, WSelectedObjectsExtractorBase);

public:
  WEditorSelectedObjectsExtractor();
  ~WEditorSelectedObjectsExtractor();

  virtual const WDeque<WGameObjectHandle>* GetSelection() override;

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetSceneContext(WSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  WSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void CreateRenderTargetTexture(const WView& view);
  void CreateRenderTargetView(const WView& view);
  void UpdateRenderTargetCamera(const WCameraComponent* pCamComp);

  WSceneContext* m_pSceneContext;
  WViewHandle m_hRenderTargetView;
  WRenderToTexture2DResourceHandle m_hRenderTarget;
  WCamera m_RenderTargetCamera;
};

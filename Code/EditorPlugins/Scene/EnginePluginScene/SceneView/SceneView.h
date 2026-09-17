#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WView;
class WViewRedrawMsgToEngine;
class WEngineProcessDocumentContext;
class WEditorEngineDocumentMsg;
class WEditorRenderPass;
class WSelectedObjectsExtractorBase;
class WSceneContext;
using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;
class WViewMarqueePickingMsgToEngine;

struct ObjectData
{
  WMat4 m_ModelView;
  float m_PickingID[4];
};

class WSceneViewContext : public WEngineProcessViewContext
{
public:
  WSceneViewContext(WSceneContext* pSceneContext);
  ~WSceneViewContext();

  virtual void HandleViewMessage(const WEditorEngineViewMsg* pMsg) override;
  virtual void SetupRenderTarget(WGALSwapChainHandle hSwapChain, const WGALRenderTargets* pRenderTargets, WUInt16 uiWidth, WUInt16 uiHeight) override;

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);
  void SetInvisibleLayerTags(const WArrayPtr<WTag> removeTags, const WArrayPtr<WTag> addTags);

protected:
  virtual void Redraw(bool bRenderEditorGizmos) override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;
  virtual void SetViewProperties(WView* pView) override;
  virtual WViewHandle CreateView() override;

  void PickObjectAt(WUInt16 x, WUInt16 y);
  void MarqueePickObjects(const WViewMarqueePickingMsgToEngine* pMsg);

private:
  WSceneContext* m_pSceneContext;

  bool m_bUpdatePickingData;

  WCamera m_CullingCamera;
};

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class WMeshContext;

class WMeshViewContext : public WEngineProcessViewContext
{
public:
  WMeshViewContext(WMeshContext* pMeshContext);
  ~WMeshViewContext();

  bool UpdateThumbnailCamera(const WBoundingBoxSphere& bounds);

  virtual void HandleViewMessage(const WEditorEngineViewMsg* pMsg) override;

protected:
  virtual WViewHandle CreateView() override;
  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg) override;

  void PickObjectAt(WUInt16 x, WUInt16 y);

  WMeshContext* m_pContext = nullptr;
  WUInt32 m_uiLastHoveredPartIndex = WInvalidIndex;
};

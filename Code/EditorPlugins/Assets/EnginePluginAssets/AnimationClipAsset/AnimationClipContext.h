#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>

class W_ENGINEPLUGINASSETS_DLL WAnimationClipContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipContext, WEngineProcessDocumentContext);

public:
  WAnimationClipContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);
  void SetPlaybackPosition(double pos);
  void GenerateAndApplyPose();
  void ExtractRootMotionFromFeet();

  WGameObject* m_pGameObject = nullptr;
  WString m_sAnimatedMeshToUse;
  WString m_sBaseAnimationClip;
  float m_fNormalizedPlaybackPosition = 0.0f;
  WComponentHandle m_hAnimMeshComponent;
};

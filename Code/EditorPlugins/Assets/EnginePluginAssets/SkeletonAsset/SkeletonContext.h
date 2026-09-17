#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Declarations.h>

class W_ENGINEPLUGINASSETS_DLL WSkeletonContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WSkeletonContext, WEngineProcessDocumentContext);

public:
  WSkeletonContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  WSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);

  WGameObject* m_pGameObject = nullptr;
  WSkeletonResourceHandle m_hSkeleton;
  WComponentHandle m_hSkeletonComponent;
  WComponentHandle m_hPoseComponent;
  WString m_sAnimatedMeshToUse;
  WComponentHandle m_hAnimMeshComponent;
};

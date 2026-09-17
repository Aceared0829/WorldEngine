#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Meshes/MeshResource.h>

class W_ENGINEPLUGINASSETS_DLL WDecalContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WDecalContext, WEngineProcessDocumentContext);

public:
  WDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;

private:
  WMeshResourceHandle m_hPreviewMeshResource;

  // WDecalResourceHandle m_hDecal;
};

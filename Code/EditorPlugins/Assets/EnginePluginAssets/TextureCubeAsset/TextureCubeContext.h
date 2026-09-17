#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINASSETS_DLL WTextureCubeContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WTextureCubeContext, WEngineProcessDocumentContext);

public:
  WTextureCubeContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WTextureCubeResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const WResourceEvent& e);

  WGameObjectHandle m_hPreviewObject;
  WComponentHandle m_hPreviewMesh2D;
  WMeshResourceHandle m_hPreviewMeshResource;
  WMaterialResourceHandle m_hMaterial;
  WTextureCubeResourceHandle m_hTexture;

  WEvent<const WResourceEvent&, WMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};

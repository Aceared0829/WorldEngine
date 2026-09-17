#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINASSETS_DLL WTextureContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WTextureContext, WEngineProcessDocumentContext);

public:
  WTextureContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WTexture2DResourceHandle& GetTexture() const { return m_hTexture; }
  int GetLodLevel() const { return m_iLodLevel; }
  WUInt32 GetNumArraySlices() const { return m_uiNumArraySlices; }

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;

private:
  void SetTexture(WStringView sTextureFile);

  /// Applies all preview parameters to the slice materials. Does nothing and leaves
  /// m_bSliceMaterialsDirty set if the materials are not loaded yet, so that it gets retried.
  void ApplySliceMaterialParameters();
  void OnResourceEvent(const WResourceEvent& e);
  void RebuildPreviewObjects(WUInt32 uiNumArraySlices);

  struct PreviewSlice
  {
    WGameObjectHandle m_hObject;
    WComponentHandle m_hMeshComponent;
    WMaterialResourceHandle m_hMaterial;
  };

  WMeshResourceHandle m_hPreviewMeshResource;
  WDynamicArray<PreviewSlice> m_SlicePreviews;

  WTexture2DResourceHandle m_hTexture;
  WEvent<const WResourceEvent&, WMutex>::Unsubscriber m_TextureResourceEventSubscriber;

  WUInt32 m_uiNumArraySlices = 0;
  WUInt32 m_uiMaterialGeneration = 0;
  bool m_bPreviewObjectsDirty = true;
  bool m_bSliceMaterialsDirty = true;
  int m_iLodLevel = -1;
  int m_iChannelMode = 0;
  float m_fAlphaThreshold = 0.0f;
};

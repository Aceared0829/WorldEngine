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

class W_ENGINEPLUGINASSETS_DLL WMaterialContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialContext, WEngineProcessDocumentContext);

public:
  WMaterialContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  WMaterialResourceHandle m_hMaterial;
  WMeshResourceHandle m_hBallMesh;
  WMeshResourceHandle m_hSphereMesh;
  WMeshResourceHandle m_hBoxMesh;
  WMeshResourceHandle m_hPlaneMesh;
  WGameObjectHandle m_hMeshObject;
  WComponentHandle m_hMeshComponent;

  enum class PreviewModel : WUInt8
  {
    Ball,
    Sphere,
    Box,
    Plane,
  };

  PreviewModel m_PreviewModel = PreviewModel::Ball;
};

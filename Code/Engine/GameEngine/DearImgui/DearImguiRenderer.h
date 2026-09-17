#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Math/Rect.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Meshes/MeshBufferResource.h>
#  include <RendererCore/Pipeline/Extractor.h>
#  include <RendererCore/Pipeline/RenderData.h>
#  include <RendererCore/Pipeline/Renderer.h>
#  include <RendererFoundation/Resources/BufferPool.h>

class WRenderDataBatch;
using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

struct WImguiVertex
{
  W_DECLARE_POD_TYPE();

  WVec3 m_Position;
  WVec2 m_TexCoord;
  WColorLinearUB m_Color;
};

struct WImguiBatch
{
  W_DECLARE_POD_TYPE();

  WRectU32 m_ScissorRect;
  ImTextureID m_TextureId;
  WUInt16 m_uiVertexCount;
};

class W_GAMEENGINE_DLL WImguiRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WImguiRenderData, WRenderData);

public:
  WArrayPtr<WImguiVertex> m_Vertices;
  WArrayPtr<ImDrawIdx> m_Indices;
  WArrayPtr<WImguiBatch> m_Batches;
};

class W_GAMEENGINE_DLL WImguiExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WImguiExtractor, WExtractor);

public:
  WImguiExtractor(const char* szName = "ImguiExtractor");

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;
};

class W_GAMEENGINE_DLL WImguiRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WImguiRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WImguiRenderer);

public:
  WImguiRenderer();
  ~WImguiRenderer();

  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

protected:
  void SetupRenderer();

  static constexpr WUInt32 s_uiVertexBufferSize = 1024 * 128;
  static constexpr WUInt32 s_uiIndexBufferSize = s_uiVertexBufferSize * 2;

  WShaderResourceHandle m_hShader;
  WGALBufferPool m_VertexBuffer;
  WGALBufferPool m_IndexBuffer;
  WSmallArray<WGALVertexAttribute, 3> m_VertexAttributes;
};

#endif

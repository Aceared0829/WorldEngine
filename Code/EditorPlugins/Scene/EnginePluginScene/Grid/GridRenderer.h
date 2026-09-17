#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererFoundation/Resources/BufferPool.h>

class WRenderDataBatch;
class WSceneContext;

using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

class WGridRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WGridRenderData, WRenderData);

public:
  WQuat m_qGlobalRotation;
  float m_fDensity;
  WInt32 m_iFirstLine1;
  WInt32 m_iLastLine1;
  WInt32 m_iFirstLine2;
  WInt32 m_iLastLine2;
  bool m_bOrthoMode;
  bool m_bGlobal;
};

class WEditorGridExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WEditorGridExtractor, WExtractor);

public:
  WEditorGridExtractor(const char* szName = "EditorGridExtractor");

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetSceneContext(WSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  WSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  WSceneContext* m_pSceneContext;
};

struct alignas(16) GridVertex
{
  WVec3 m_position;
  WColorLinearUB m_color;
};

class WGridRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WGridRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WGridRenderer);

public:
  WGridRenderer();

  // WRenderer implementation
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

protected:
  void CreateVertexBuffer();

  static constexpr WUInt32 s_uiBufferSize = 1024 * 8;
  static constexpr WUInt32 s_uiLineVerticesPerBatch = s_uiBufferSize / sizeof(GridVertex);

  WShaderResourceHandle m_hShader;
  WGALBufferPool m_VertexBuffer;
  WSmallArray<WGALVertexAttribute, 2> m_VertexAttributes;
  mutable WDynamicArray<GridVertex, WAlignedAllocatorWrapper> m_Vertices;

private:
  void CreateGrid(const WGridRenderData& rd) const;
};

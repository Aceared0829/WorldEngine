#pragma once

#include <RendererCore/Pipeline/Renderer.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

class WRenderDataBatch;

/// Renders terrain patches submitted as WTerrainHeightfieldRenderData.
///
/// Each patch is drawn as a procedural grid using SV_VertexID. No vertex buffer is needed;
/// the vertex shader reads height data from a structured buffer (SRV) bound per patch.
/// The shader and textures come from the material assigned to the patch component.
class W_TERRAINPLUGIN_DLL WTerrainHeightfieldRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WTerrainHeightfieldRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WTerrainHeightfieldRenderer);

public:
  WTerrainHeightfieldRenderer();
  ~WTerrainHeightfieldRenderer();

  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;
};

/// Renders voxel mesh volumes directly from the GPU buffers produced by surface nets shaders.
///
/// No CPU vertex/index upload is required. The vertex shader reads VoxelGpuVertex structs from a
/// StructuredBuffer SRV and uses the index SRV to emulate indexed drawing, both indexed by SV_VertexID.
class W_TERRAINPLUGIN_DLL WTerrainVoxelRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WTerrainVoxelRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WTerrainVoxelRenderer);

public:
  WTerrainVoxelRenderer();
  ~WTerrainVoxelRenderer();

  void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  void RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;
};

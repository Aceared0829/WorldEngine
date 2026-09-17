#include <TerrainPlugin/TerrainPluginPCH.h>

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>
#include <Shaders/Terrain/Rendering/HeightfieldRenderConstants.h>
#include <Shaders/Terrain/Rendering/VoxelMeshRenderConstants.h>
#include <TerrainPlugin/Rendering/TerrainRenderData.h>
#include <TerrainPlugin/Rendering/TerrainRenderer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainHeightfieldRenderData, 1, WRTTIDefaultAllocator<WTerrainHeightfieldRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainHeightfieldRenderer, 1, WRTTIDefaultAllocator<WTerrainHeightfieldRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTerrainHeightfieldRenderer::WTerrainHeightfieldRenderer() = default;
WTerrainHeightfieldRenderer::~WTerrainHeightfieldRenderer() = default;

void WTerrainHeightfieldRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WTerrainHeightfieldRenderData>());
}

void WTerrainHeightfieldRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WRenderContext* pContext = renderViewContext.m_pRenderContext;

  for (auto it = batch.GetIterator<WTerrainHeightfieldRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const WTerrainHeightfieldRenderData* pRenderData = it;

    if (pRenderData->m_hHeightBuffer.IsInvalidated() || pRenderData->m_hCellMaterialBuffer.IsInvalidated() || !pRenderData->m_hMaterial.IsValid())
      continue;

    // Shader and textures come from the material asset assigned to the component.
    pContext->BindMaterial(pRenderData->m_hMaterial);

    // Bind height and normal buffers as SRVs — the shader reads them by vertex index
    WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
    bindGroup.BindBuffer("TerrainHeights", pRenderData->m_hHeightBuffer);
    bindGroup.BindBuffer("TerrainNormals", pRenderData->m_hNormalBuffer);
    bindGroup.BindBuffer("TerrainCellMaterials", pRenderData->m_hCellMaterialBuffer);
    bindGroup.BindBuffer("TerrainWeights", pRenderData->m_hVertexWeightBuffer);
    bindGroup.BindBuffer("TerrainCarveMask", pRenderData->m_hCarveMaskBuffer);

    // Bind instance data buffer so the pixel shader can read GameObjectID for picking.
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    if (auto* pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
    {
      bindGroup.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
    }

    const WUInt32 uiCellsFull = pRenderData->m_uiCellsPerSide;

    // +1 for vertex count (cells+1 vertices per side), +8 for 4-vertex border on each side.
    const WUInt32 uiStoredSize = uiCellsFull + 9;

    // LOD: step over 2^LOD stored vertices between rendered corners. Reduce the step if it would not
    // divide the grid evenly (keeps the reduced grid aligned to the full-resolution vertices).
    WUInt32 uiStep = 1u << WMath::Min<WUInt32>(pRenderData->m_uiLod, 2u);
    while (uiStep > 1 && (uiCellsFull % uiStep) != 0)
      uiStep >>= 1;

    const WUInt32 uiInnerCells = uiCellsFull / uiStep;

    // The border ring is 4 stored vertices wide; at LOD step that maps to 4 / step skirt cells per side.
    const WUInt32 uiSkirtCells = pRenderData->m_bRenderSkirt ? (4u / uiStep) : 0u;
    const WUInt32 uiRenderCells = uiInnerCells + 2u * uiSkirtCells;

    HeightfieldRenderConstants constants;
    constants.GridSpacing = pRenderData->m_fGridSpacing;
    constants.FirstVertexIdx = 4 * uiStoredSize + 4; // skip 4 border rows and 4 border cols
    constants.VertexIdxPitch = uiStoredSize;
    constants.CellsPerSide = uiCellsFull;
    constants.InstanceDataOffset = pRenderData->m_DataOffsets.m_uiInstance;
    constants.FallbackMaterialSlot = pRenderData->m_uiDefaultMaterialIndex;
    constants.RenderCellsPerSide = uiRenderCells;
    constants.VertexStep = uiStep;
    constants.SkirtCells = uiSkirtCells;
    constants.SkirtDepth = pRenderData->m_bRenderSkirt ? pRenderData->m_fSkirtDepth : 0.0f;
    constants.LodFade = pRenderData->m_fLodFade;
    pContext->SetPushConstants("HeightfieldRenderConstants", constants);

    // No vertex buffer — SV_VertexID drives everything.
    const WUInt32 uiPrimitiveCount = uiRenderCells * uiRenderCells * 2;
    pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, uiPrimitiveCount);

    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainVoxelRenderData, 1, WRTTIDefaultAllocator<WTerrainVoxelRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainVoxelRenderer, 1, WRTTIDefaultAllocator<WTerrainVoxelRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTerrainVoxelRenderer::WTerrainVoxelRenderer() = default;
WTerrainVoxelRenderer::~WTerrainVoxelRenderer() = default;

void WTerrainVoxelRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WTerrainVoxelRenderData>());
}

void WTerrainVoxelRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WRenderContext* pContext = renderViewContext.m_pRenderContext;

  const bool bAsync = pContext->GetAllowAsyncShaderLoading();
  pContext->SetAllowAsyncShaderLoading(false);
  W_SCOPE_EXIT(pContext->SetAllowAsyncShaderLoading(bAsync));

  for (auto it = batch.GetIterator<WTerrainVoxelRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const WTerrainVoxelRenderData* pRenderData = it;

    if (pRenderData->m_hGpuMeshVertices.IsInvalidated() || pRenderData->m_hGpuMeshIndices.IsInvalidated() || pRenderData->m_hGpuMeshDrawArgs.IsInvalidated() || !pRenderData->m_hMaterial.IsValid())
      continue;

    WResourceLock<WMaterialResource> pMaterial(pRenderData->m_hMaterial, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
      continue;

    // Material provides the shader (VoxelMeshMaterial.WShader) and any texture bindings.
    pContext->BindMaterial(pRenderData->m_hMaterial);

    // Bind GPU vertex + index buffers as SRVs.
    // The VS reads: index = VoxelIndices[SV_VertexID], then vertex = VoxelVertices[index].
    WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
    bindGroup.BindBuffer("VoxelVertices", pRenderData->m_hGpuMeshVertices);
    bindGroup.BindBuffer("VoxelIndices", pRenderData->m_hGpuMeshIndices);

    // Bind instance data buffer so the shader can read the world transform and GameObjectID.
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    if (auto* pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
    {
      bindGroup.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
    }

    VoxelMeshRenderConstants constants;
    constants.InstanceDataOffset = pRenderData->m_DataOffsets.m_uiInstance;
    constants.BaseMaterialIndex = pRenderData->m_uiBaseMaterialIndex;
    pContext->SetPushConstants("VoxelMeshRenderConstants", constants);

    // Non-indexed draw: SV_VertexID drives index + vertex lookup.
    // Vertex count is read from the GPU-side indirect args buffer (filled in the same submission as the mesh).
    pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    if (pContext->ApplyContextStates().Succeeded())
    {
      pContext->GetCommandEncoder()->DrawInstancedIndirect(pRenderData->m_hGpuMeshDrawArgs, 0).IgnoreResult();
    }
  }
}

W_STATICLINK_FILE(TerrainPlugin, TerrainPlugin_Rendering_TerrainRenderer);

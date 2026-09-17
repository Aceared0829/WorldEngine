#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshRenderer, 1, WRTTIDefaultAllocator<WMeshRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMeshRenderer::WMeshRenderer() = default;
WMeshRenderer::~WMeshRenderer() = default;

void WMeshRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WMeshRenderData>());
  out_types.PushBack(WGetStaticRTTI<WCustomMeshRenderData>());
  out_types.PushBack(WGetStaticRTTI<WSkinnedMeshRenderData>());
}

void WMeshRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WRenderContext* pContext = renderViewContext.m_pRenderContext;
  auto& bg = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);

  const WInstanceableRenderData* pRenderData = batch.GetFirstData<WInstanceableRenderData>();

  WUInt32 uiPrimitiveCount = 0;
  WUInt32 uiFirstPrimitive = 0;
  bool bUseSkinning = false;
  if (pRenderData->IsInstanceOf<WCustomMeshRenderData>())
  {
    const auto* pCustomMeshRenderData = static_cast<const WCustomMeshRenderData*>(pRenderData);
    uiPrimitiveCount = pCustomMeshRenderData->m_uiNumPrimitives;
    uiFirstPrimitive = pCustomMeshRenderData->m_uiFirstPrimitive;

    pContext->BindMeshBuffer(pCustomMeshRenderData->m_hDynamicMeshBuffer, batch.GetDataOffsetsBuffer(), batch.GetFirstDataOffsetIndex());
    pContext->BindMaterial(pCustomMeshRenderData->m_hMaterial);
  }
  else
  {
    const auto* pMeshRenderData = static_cast<const WMeshRenderData*>(pRenderData);
    const WUInt32 uiPartIndex = pMeshRenderData->m_uiSubMeshIndex;

    const WMeshResourceHandle& hMesh = pMeshRenderData->m_hMesh;
    WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::AllowLoadingFallback);

    // This can happen when the resource has been reloaded and now has fewer submeshes.
    const auto& subMeshes = pMesh->GetSubMeshes();
    if (subMeshes.GetCount() <= uiPartIndex)
    {
      return;
    }

    const WMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];
    uiPrimitiveCount = meshPart.m_uiPrimitiveCount;
    uiFirstPrimitive = meshPart.m_uiFirstPrimitive;

    pContext->BindMeshBuffer(pMesh->GetMeshBuffer(), batch.GetDataOffsetsBuffer(), batch.GetFirstDataOffsetIndex());
    pContext->BindMaterial(pMeshRenderData->m_hMaterial);

    if (auto pCustomInstanceDataBuffer = pDevice->GetDynamicBuffer(pMeshRenderData->m_hCustomInstanceDataBuffer))
    {
      bg.BindBuffer("perInstanceDataCustom", pCustomInstanceDataBuffer->GetBufferForRendering());
    }

    if (auto pSkinnedMeshRenderData = WDynamicCast<const WSkinnedMeshRenderData*>(pRenderData))
    {
      if (auto pSkinningBuffer = pDevice->GetDynamicBuffer(pSkinnedMeshRenderData->m_hSkinningBuffer))
      {
        bg.BindBuffer("skinningTransforms", pSkinningBuffer->GetBufferForRendering());
        bUseSkinning = true;
      }
    }

    SetAdditionalData(renderViewContext, pMeshRenderData);
  }

  constexpr WTempHashedString sTrue("TRUE");
  constexpr WTempHashedString sFalse("FALSE");

  pContext->SetShaderPermutationVariable("VERTEX_SKINNING", bUseSkinning ? sTrue : sFalse);
  pContext->SetShaderPermutationVariable("FLIP_WINDING", pRenderData->FlipWinding() ? sTrue : sFalse);

  if (auto pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
  {
    bg.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
  }

  if (pContext->DrawMeshBuffer(uiPrimitiveCount, uiFirstPrimitive, batch.GetInstanceCount()).Failed())
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    // draw bounding box instead
    for (auto it = batch.GetIterator<WRenderData>(); it.IsValid(); ++it)
    {
      if (auto pMeshRenderData = WDynamicCast<const WMeshRenderData*>(it))
      {
        if (pMeshRenderData->m_FallbackGlobalBBox.IsValid())
        {
          WDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pMeshRenderData->m_FallbackGlobalBBox, WColor::Magenta);
        }
      }
      else if (auto pCustomMeshRenderData = WDynamicCast<const WCustomMeshRenderData*>(it))
      {
        // draw bounding box instead
        if (pCustomMeshRenderData->m_FallbackGlobalBBox.IsValid())
        {
          WDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pCustomMeshRenderData->m_FallbackGlobalBBox, WColor::Magenta);
        }
      }
    }
#endif
  }
}

void WMeshRenderer::SetAdditionalData(const WRenderViewContext& renderViewContext, const WMeshRenderData* pRenderData) const
{
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Components/LensFlareRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/LensFlareData.h>
static_assert(sizeof(WPerLensFlareData) == 48);

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLensFlareRenderer, 1, WRTTIDefaultAllocator<WLensFlareRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;

WLensFlareRenderer::WLensFlareRenderer()
{
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Materials/LensFlareMaterial.WShader");
}

WLensFlareRenderer::~WLensFlareRenderer() = default;

void WLensFlareRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WLensFlareRenderData>());
}

void WLensFlareRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WRenderContext* pContext = renderViewContext.m_pRenderContext;

  const WLensFlareRenderData* pRenderData = batch.GetFirstData<WLensFlareRenderData>();

  const WUInt32 uiBufferSize = WMath::RoundUp(batch.GetDataCount(), 128u);
  WGALBufferHandle hLensFlareData = CreateLensFlareDataBuffer(uiBufferSize);
  W_SCOPE_EXIT(DeleteLensFlareDataBuffer(hLensFlareData));

  WBindGroupBuilder& bindGroupRenderPass = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
  pContext->BindShader(m_hShader);
  bindGroupRenderPass.BindBuffer("lensFlareData", hLensFlareData);
  bindGroupRenderPass.BindTexture("LensFlareTexture", pRenderData->m_hTexture);

  FillLensFlareData(batch);

  if (m_LensFlareData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hLensFlareData, 0, m_LensFlareData.GetByteArrayPtr(), WGALUpdateMode::AheadOfTime);

    pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, m_LensFlareData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

WGALBufferHandle WLensFlareRenderer::CreateLensFlareDataBuffer(WUInt32 uiBufferSize) const
{
  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(WPerLensFlareData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return WGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void WLensFlareRenderer::DeleteLensFlareDataBuffer(WGALBufferHandle hBuffer) const
{
  WGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void WLensFlareRenderer::FillLensFlareData(const WRenderDataBatch& batch) const
{
  m_LensFlareData.Clear();
  m_LensFlareData.Reserve(batch.GetDataCount());

  for (auto it = batch.GetIterator<WLensFlareRenderData>(); it.IsValid(); ++it)
  {
    const WLensFlareRenderData* pRenderData = it;

    auto& LensFlareData = m_LensFlareData.ExpandAndGetRef();
    LensFlareData.WorldSpacePosition = pRenderData->m_vGlobalPosition;
    LensFlareData.Size = pRenderData->m_fSize;
    LensFlareData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    LensFlareData.OcclusionRadius = pRenderData->m_fOcclusionSampleRadius;
    LensFlareData.OcclusionSpread = pRenderData->m_fOcclusionSampleSpread;
    LensFlareData.DepthOffset = pRenderData->m_fOcclusionDepthOffset;
    LensFlareData.AspectRatioAndShift = WShaderUtils::Float2ToRG16F(WVec2(pRenderData->m_fAspectRatio, pRenderData->m_fShiftToCenter));
    LensFlareData.ColorRG = WShaderUtils::PackFloat16intoUint(pRenderData->m_Color.x, pRenderData->m_Color.y);
    LensFlareData.ColorBA = WShaderUtils::PackFloat16intoUint(pRenderData->m_Color.z, pRenderData->m_Color.w);
    LensFlareData.Flags = (pRenderData->m_bInverseTonemap ? LENS_FLARE_INVERSE_TONEMAP : 0) |
                          (pRenderData->m_bGreyscaleTexture ? LENS_FLARE_GREYSCALE_TEXTURE : 0) |
                          (pRenderData->m_bApplyFog ? LENS_FLARE_APPLY_FOG : 0);
  }
}



W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareRenderer);

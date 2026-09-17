#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/ParticleSystemConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticlePointRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticlePointRenderer, 1, WRTTIDefaultAllocator<WParticlePointRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticlePointRenderer::WParticlePointRenderer()
{
  CreateParticleDataBuffer(m_BaseDataBuffer, sizeof(WBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_BillboardDataBuffer, sizeof(WBillboardQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Particles/Point.WShader");
}


WParticlePointRenderer::~WParticlePointRenderer()
{
  DestroyParticleDataBuffer(m_BaseDataBuffer);
  DestroyParticleDataBuffer(m_BillboardDataBuffer);
}

void WParticlePointRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WParticlePointRenderData>());
}

void WParticlePointRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // Bind mesh buffer
  {
    pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Points, s_uiParticlesPerBatch);
  }

  // now render all particle effects of type Point
  for (auto it = batch.GetIterator<WParticlePointRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const WParticlePointRenderData* pRenderData = it;

    const WBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const WBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();

    WUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    systemConstants.SetGenericData(pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, 1, 1, 1, 1);

    while (uiNumParticles > 0)
    {
      // Request new buffers and bind them
      WGALBufferHandle hBaseDataBuffer = m_BaseDataBuffer.GetNewBuffer();
      WGALBufferHandle hBillboardDataBuffer = m_BillboardDataBuffer.GetNewBuffer();
      WBindGroupBuilder& bindGroupDraw = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
      bindGroupDraw.BindBuffer("particleBaseData", hBaseDataBuffer);
      bindGroupDraw.BindBuffer("particleBillboardQuadData", hBillboardDataBuffer);

      // upload this batch of particle data
      const WUInt32 uiNumParticlesInBatch = WMath::Min<WUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBaseDataBuffer, 0, WMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBillboardDataBuffer, 0, WMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticleBillboardData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch).IgnoreResult();
    }
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_PointRenderer);

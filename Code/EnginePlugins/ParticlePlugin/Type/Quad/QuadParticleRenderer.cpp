#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleQuadRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleQuadRenderer, 1, WRTTIDefaultAllocator<WParticleQuadRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WParticleQuadRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WParticleQuadRenderData&>(other0);

  return m_RenderMode == other.m_RenderMode && m_hTexture == other.m_hTexture && m_hCustomMaterial == other.m_hCustomMaterial;
}

//////////////////////////////////////////////////////////////////////////

WParticleQuadRenderer::WParticleQuadRenderer()
{
  CreateParticleDataBuffer(m_BaseDataBuffer, sizeof(WBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_BillboardDataBuffer, sizeof(WBillboardQuadParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TangentDataBuffer, sizeof(WTangentQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Particles/DefaultQuadParticle.WShader");
}

WParticleQuadRenderer::~WParticleQuadRenderer()
{
  DestroyParticleDataBuffer(m_BaseDataBuffer);
  DestroyParticleDataBuffer(m_BillboardDataBuffer);
  DestroyParticleDataBuffer(m_TangentDataBuffer);
}

void WParticleQuadRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WParticleQuadRenderData>());
}

void WParticleQuadRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  bool bBindShader = true;

  // Bind mesh buffer
  {
    pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, s_uiParticlesPerBatch * 2);
  }

  WBindGroupBuilder& bindGroupMaterial = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);

  // now render all particle effects of type Quad
  for (auto it = batch.GetIterator<WParticleQuadRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const WParticleQuadRenderData* pRenderData = it;

    if (pRenderData->m_hCustomMaterial.IsValid())
    {
      WResourceLock<WMaterialResource> pMat(pRenderData->m_hCustomMaterial, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
      if (pMat.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        // skip rendering this particle effect in case the custom material is not yet loaded (or fails to load)
        // otherwise we would get the fallback material, which doesn't work with particle vertex streams
        return;
      }

      pRenderContext->BindMaterial(pRenderData->m_hCustomMaterial);
    }
    else
    {
      if (bBindShader)
      {
        bBindShader = false;
        pRenderContext->BindShader(m_hShader);
      }

      bindGroupMaterial.BindTexture("ParticleTexture", pRenderData->m_hTexture);
    }

    const WBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const WBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();
    const WTangentQuadParticleShaderData* pParticleTangentData = pRenderData->m_TangentParticleData.GetPtr();

    WUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    ConfigureRenderMode(pRenderData, pRenderContext);

    systemConstants.SetGenericData(pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fNormalCurvature, pRenderData->m_fLightDirectionality, pRenderData->m_fGeometryProximityFadeOut, pRenderData->m_fCameraProximityFadeOut, pRenderData->m_TextureAtlasOrientation.GetValue());

    pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", pRenderData->m_QuadModePermutation);

    while (uiNumParticles > 0)
    {
      // Request new buffers and bind them
      WGALBufferHandle hBaseDataBuffer = m_BaseDataBuffer.GetNewBuffer();
      WGALBufferHandle hBillboardDataBuffer = m_BillboardDataBuffer.GetNewBuffer();
      WGALBufferHandle hTangentDataBuffer = m_TangentDataBuffer.GetNewBuffer();

      WBindGroupBuilder& bindGroupDraw = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
      bindGroupDraw.BindBuffer("particleBaseData", hBaseDataBuffer);
      bindGroupDraw.BindBuffer("particleBillboardQuadData", hBillboardDataBuffer);
      bindGroupDraw.BindBuffer("particleTangentQuadData", hTangentDataBuffer);

      // upload this batch of particle data
      const WUInt32 uiNumParticlesInBatch = WMath::Min<WUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBaseDataBuffer, 0, WMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticleBaseData += uiNumParticlesInBatch;

      if (pParticleBillboardData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(hBillboardDataBuffer, 0, WMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
        pParticleBillboardData += uiNumParticlesInBatch;
      }

      if (pParticleTangentData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(hTangentDataBuffer, 0, WMakeArrayPtr(pParticleTangentData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
        pParticleTangentData += uiNumParticlesInBatch;
      }

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * 2).IgnoreResult();
    }
  }
}

void WParticleQuadRenderer::ConfigureRenderMode(const WParticleQuadRenderData* pRenderData, WRenderContext* pRenderContext) const
{
  switch (pRenderData->m_RenderMode)
  {
    case WParticleTypeRenderMode::Additive:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case WParticleTypeRenderMode::Blended:
    case WParticleTypeRenderMode::BlendedForeground:
    case WParticleTypeRenderMode::BlendedBackground:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case WParticleTypeRenderMode::Opaque:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;

    case WParticleTypeRenderMode::Unused:
    case WParticleTypeRenderMode::Unused2:
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  switch (pRenderData->m_LightingMode)
  {
    case WParticleLightingMode::Fullbright:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_LIGHTING_MODE", "PARTICLE_LIGHTING_MODE_FULLBRIGHT");
      break;
    case WParticleLightingMode::VertexLit:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_LIGHTING_MODE", "PARTICLE_LIGHTING_MODE_VERTEX_LIT");
      break;
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_QuadParticleRenderer);

#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/ParticleSystemConstants.h>

//clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTrailRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTrailRenderer, 1, WRTTIDefaultAllocator<WParticleTrailRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WParticleTrailRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WParticleTrailRenderData&>(other0);

  return m_RenderMode == other.m_RenderMode && m_hTexture == other.m_hTexture && m_uiMaxTrailPoints == other.m_uiMaxTrailPoints && m_hCustomMaterial == other.m_hCustomMaterial;
}

//////////////////////////////////////////////////////////////////////////

WParticleTrailRenderer::WParticleTrailRenderer()
{
  CreateParticleDataBuffer(m_BaseDataBuffer, sizeof(WBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TrailDataBuffer, sizeof(WTrailParticleShaderData), s_uiParticlesPerBatch);

  // this is kinda stupid, apparently due to stride enforcement I cannot reuse the same buffer for different sizes
  // and instead have to create one buffer with every size ...

  CreateParticleDataBuffer(m_TrailPointsDataBuffer8, sizeof(WTrailParticlePointsData8), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TrailPointsDataBuffer16, sizeof(WTrailParticlePointsData16), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TrailPointsDataBuffer32, sizeof(WTrailParticlePointsData32), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TrailPointsDataBuffer64, sizeof(WTrailParticlePointsData64), s_uiParticlesPerBatch);

  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Particles/DefaultTrailParticle.WShader");
}

WParticleTrailRenderer::~WParticleTrailRenderer()
{
  DestroyParticleDataBuffer(m_BaseDataBuffer);
  DestroyParticleDataBuffer(m_TrailDataBuffer);
  DestroyParticleDataBuffer(m_TrailPointsDataBuffer8);
  DestroyParticleDataBuffer(m_TrailPointsDataBuffer16);
  DestroyParticleDataBuffer(m_TrailPointsDataBuffer32);
  DestroyParticleDataBuffer(m_TrailPointsDataBuffer64);
}

void WParticleTrailRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WParticleTrailRenderData>());
}

void WParticleTrailRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  WGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  bool bBindShader = true;

  WBindGroupBuilder& bindGroupMaterial = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);

  // now render all particle effects of type Trail
  for (auto it = batch.GetIterator<WParticleTrailRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const WParticleTrailRenderData* pRenderData = it;

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


    if (!ConfigureShader(pRenderData, renderViewContext))
      continue;

    const WUInt32 uiBucketSize = WParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints);
    const WUInt32 uiMaxTrailSegments = uiBucketSize - 1;
    const WUInt32 uiPrimFactor = 2;
    const WUInt32 uiMaxPrimitivesToRender = s_uiParticlesPerBatch * uiMaxTrailSegments * uiPrimFactor;


    pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, uiMaxPrimitivesToRender);

    const WBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const WTrailParticleShaderData* pParticleTrailData = pRenderData->m_TrailParticleData.GetPtr();


    const WVec4* pParticlePointsData = pRenderData->m_TrailPointsShared.GetPtr();

    bindGroupMaterial.BindTexture("ParticleTexture", pRenderData->m_hTexture);

    systemConstants.SetGenericData(pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fNormalCurvature, pRenderData->m_fLightDirectionality, 0.1f, 0.5f, pRenderData->m_TextureAtlasOrientation.GetValue());
    systemConstants.SetTrailData(pRenderData->m_fSnapshotFraction, pRenderData->m_uiMaxTrailPoints);

    WUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();
    while (uiNumParticles > 0)
    {
      // Request and bind new buffers for this batch
      WGALBufferHandle hBaseDataBuffer = m_BaseDataBuffer.GetNewBuffer();
      WGALBufferHandle hTrailDataBuffer = m_TrailDataBuffer.GetNewBuffer();
      WGALBufferHandle hActiveTrailPointsDataBuffer = m_pActiveTrailPointsDataBuffer->GetNewBuffer();

      WBindGroupBuilder& bindGroupDraw = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
      bindGroupDraw.BindBuffer("particleBaseData", hBaseDataBuffer);
      bindGroupDraw.BindBuffer("particleTrailData", hTrailDataBuffer);
      bindGroupDraw.BindBuffer("particlePointsData", hActiveTrailPointsDataBuffer);

      // upload this batch of particle data
      const WUInt32 uiNumParticlesInBatch = WMath::Min<WUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBaseDataBuffer, 0, WMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hTrailDataBuffer, 0, WMakeArrayPtr(pParticleTrailData, uiNumParticlesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticleTrailData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hActiveTrailPointsDataBuffer, 0, WMakeArrayPtr(pParticlePointsData, uiNumParticlesInBatch * uiBucketSize).ToByteArray(), WGALUpdateMode::AheadOfTime);
      pParticlePointsData += uiNumParticlesInBatch * uiBucketSize;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * uiMaxTrailSegments * uiPrimFactor).IgnoreResult();
    }
  }
}

bool WParticleTrailRenderer::ConfigureShader(const WParticleTrailRenderData* pRenderData, const WRenderViewContext& renderViewContext) const
{
  auto pRenderContext = renderViewContext.m_pRenderContext;

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

  switch (WParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints))
  {
    case 8:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT8");
      m_pActiveTrailPointsDataBuffer = &m_TrailPointsDataBuffer8;
      break;
    case 16:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT16");
      m_pActiveTrailPointsDataBuffer = &m_TrailPointsDataBuffer16;
      break;
    case 32:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT32");
      m_pActiveTrailPointsDataBuffer = &m_TrailPointsDataBuffer32;
      break;
    case 64:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT64");
      m_pActiveTrailPointsDataBuffer = &m_TrailPointsDataBuffer64;
      break;

    default:
      return false;
  }

  return true;
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_TrailRenderer);

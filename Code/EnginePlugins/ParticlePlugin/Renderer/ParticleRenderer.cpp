#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/BufferPool.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleRenderer, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleRenderer::TempSystemCB::TempSystemCB(WRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage(m_pConstants);

  WBindGroupBuilder& bindGroupMaterial = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
  bindGroupMaterial.BindBuffer("WParticleSystemConstants", m_hConstantBuffer);
}

WParticleRenderer::TempSystemCB::~TempSystemCB()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void WParticleRenderer::TempSystemCB::SetGenericData(const WTransform& objectTransform, WTime effectLifeTime, WUInt8 uiNumVariationsX, WUInt8 uiNumVariationsY, WUInt8 uiNumFlipbookAnimsX, WUInt8 uiNumFlipbookAnimsY, float fNormalCurvature, float fLightDirectionality, float fGeometryProximityFadeOut, float fCameraProximityFadeOut, WUInt8 uiTextureAtlasOrientation)
{
  WParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.ObjectToWorldMatrix = objectTransform.GetAsMat4();
  cb.TextureAtlasVariationFramesX = uiNumVariationsX;
  cb.TextureAtlasVariationFramesY = uiNumVariationsY;
  cb.TextureAtlasFlipbookFramesX = uiNumFlipbookAnimsX;
  cb.TextureAtlasFlipbookFramesY = uiNumFlipbookAnimsY;
  cb.TotalEffectLifeTime = effectLifeTime.AsFloatInSeconds();
  cb.NormalCurvature = fNormalCurvature;
  cb.LightDirectionality = fLightDirectionality;
  cb.GeometryProximityFadeOut = fGeometryProximityFadeOut;
  cb.CameraProximityFadeOut = fCameraProximityFadeOut;
  cb.TextureAtlasOrientation = uiTextureAtlasOrientation;
}


void WParticleRenderer::TempSystemCB::SetTrailData(float fSnapshotFraction, WInt32 iNumUsedTrailPoints)
{
  WParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.SnapshotFraction = fSnapshotFraction;
  cb.NumUsedTrailPoints = iNumUsedTrailPoints;
}

WParticleRenderer::WParticleRenderer() = default;
WParticleRenderer::~WParticleRenderer() = default;

void WParticleRenderer::CreateParticleDataBuffer(WGALBufferPool& inout_Buffer, WUInt32 uiDataTypeSize, WUInt32 uiNumParticlesPerBatch)
{
  if (!inout_Buffer.IsInitialized())
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiDataTypeSize;
    desc.m_uiTotalSize = uiNumParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    inout_Buffer.Initialize(desc, "ParticleRenderer - StructuredBuffer");
  }
}


void WParticleRenderer::DestroyParticleDataBuffer(WGALBufferPool& inout_Buffer)
{
  if (inout_Buffer.IsInitialized())
  {
    inout_Buffer.Deinitialize();
  }
}

void WParticleRenderer::BindParticleShader(WRenderContext* pRenderContext, const char* szShader) const
{
  if (!m_hShader.IsValid())
  {
    // m_hShader = WResourceManager::LoadResource<WShaderResource>(szShader);
  }

  pRenderContext->BindShader(m_hShader);
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);

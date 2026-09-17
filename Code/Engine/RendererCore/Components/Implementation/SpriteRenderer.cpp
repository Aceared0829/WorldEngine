#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/SpriteData.h>
static_assert(sizeof(WPerSpriteData) == 48);

float WSpriteRenderer::s_fShapeIconScale = 1.0f;
float WSpriteRenderer::s_fShapeIconFadeDistance = 100.0f;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpriteRenderer, 1, WRTTIDefaultAllocator<WSpriteRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSpriteRenderer::WSpriteRenderer()
{
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Materials/SpriteMaterial.WShader");
}

WSpriteRenderer::~WSpriteRenderer() = default;

void WSpriteRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WSpriteRenderData>());
}

void WSpriteRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WRenderContext* pContext = renderViewContext.m_pRenderContext;

  const WSpriteRenderData* pRenderData = batch.GetFirstData<WSpriteRenderData>();

  const WUInt32 uiBufferSize = WMath::RoundUp(batch.GetDataCount(), 128u);
  WGALBufferHandle hSpriteData = CreateSpriteDataBuffer(uiBufferSize);
  W_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
  bindGroupRenderPass.BindBuffer("spriteData", hSpriteData);
  bindGroupRenderPass.BindTexture("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", WSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == WSpriteBlendMode::ShapeIcon ? WMakeHashedString("TRUE") : WMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr(), WGALUpdateMode::AheadOfTime);

    pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, m_SpriteData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

WGALBufferHandle WSpriteRenderer::CreateSpriteDataBuffer(WUInt32 uiBufferSize) const
{
  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(WPerSpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return WGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void WSpriteRenderer::DeleteSpriteDataBuffer(WGALBufferHandle hBuffer) const
{
  WGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void WSpriteRenderer::FillSpriteData(const WRenderDataBatch& batch) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(batch.GetDataCount());

  for (auto it = batch.GetIterator<WSpriteRenderData>(); it.IsValid(); ++it)
  {
    const WSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_vGlobalPosition;
    spriteData.Size = pRenderData->m_fSize;
    spriteData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio = pRenderData->m_fAspectRatio;
    spriteData.ColorRG = WShaderUtils::PackFloat16intoUint(pRenderData->m_color.r, pRenderData->m_color.g);
    spriteData.ColorBA = WShaderUtils::PackFloat16intoUint(pRenderData->m_color.b, pRenderData->m_color.a);
    spriteData.TexCoordScale = WShaderUtils::PackFloat16intoUint(pRenderData->m_texCoordScale.x, pRenderData->m_texCoordScale.y);
    spriteData.TexCoordOffset = WShaderUtils::PackFloat16intoUint(pRenderData->m_texCoordOffset.x, pRenderData->m_texCoordOffset.y);
    spriteData.GameObjectID = pRenderData->m_uiUniqueID;
    spriteData.ShapeIconFadeDistance = s_fShapeIconFadeDistance;
  }
}



W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);

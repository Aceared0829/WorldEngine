#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

namespace
{
  static WShaderResourceHandle g_hDownscaleShader;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderGraphUtils)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "ResourceManager",
  "RenderGraphManager"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  g_hDownscaleShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Downscale.WShader");
  W_ASSERT_DEV(g_hDownscaleShader.IsValid(), "Could not load Downscale shader.");
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  g_hDownscaleShader.Invalidate();
}

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WRenderGraphTextureHandle WRenderGraphUtils::GenerateMipMaps(WGALTextureHandle hTexture, WGALTextureRange range, WRenderGraph& ref_renderGraph)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  W_ASSERT_DEV(pDevice != nullptr, "No GAL device available.");

  const WGALTexture* pTexture = pDevice->GetTexture(hTexture);
  W_ASSERT_DEV(pTexture != nullptr, "GenerateMipMaps called with invalid texture handle.");
  if (pTexture == nullptr)
    return {};

  const WGALTextureCreationDescription& desc = pTexture->GetDescription();
  W_ASSERT_DEV(desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget), "RenderTarget usage required to create mip maps");
  W_ASSERT_DEV(desc.m_SampleCount == WGALMSAASampleCount::None, "Generating mipmaps for MSAA textures is not supported.");
  W_ASSERT_DEV(desc.m_uiMipLevelCount > 1, "Texture has no mip chain to generate.");
  W_ASSERT_DEV(!WGALResourceFormat::IsDepthFormat(desc.m_Format), "Generating mipmaps for depth textures is not supported.");
  W_ASSERT_DEV(desc.m_Type == WGALTextureType::Texture2D || desc.m_Type == WGALTextureType::Texture2DArray || desc.m_Type == WGALTextureType::TextureCube,
    "GenerateMipMaps currently supports Texture2D, Texture2DArray and TextureCube only.");

  range = pTexture->ClampRange(range);

  WRenderGraphTextureHandle hGraphTexture = ref_renderGraph.ImportTexture(hTexture);

  if (range.m_uiMipLevels <= 1)
    return hGraphTexture;

  static WHashedString sCameraMode = WMakeHashedString("CAMERA_MODE");
  static WHashedString sPerspective = WMakeHashedString("CAMERA_MODE_PERSPECTIVE");

  const WUInt32 uiArraySliceEnd = range.m_uiBaseArraySlice + range.m_uiArraySlices;
  const WUInt32 uiMipEnd = range.m_uiBaseMipLevel + range.m_uiMipLevels;

  ref_renderGraph.PushMarker("GenerateMipMaps");
  for (WUInt32 uiArraySlice = range.m_uiBaseArraySlice; uiArraySlice < uiArraySliceEnd; ++uiArraySlice)
  {
    for (WUInt32 uiMipLevel = range.m_uiBaseMipLevel; uiMipLevel < uiMipEnd - 1; ++uiMipLevel)
    {
      const WUInt8 uiSourceMip = static_cast<WUInt8>(uiMipLevel);
      const WUInt8 uiTargetMip = static_cast<WUInt8>(uiMipLevel + 1);
      const WUInt16 uiSlice = static_cast<WUInt16>(uiArraySlice);

      WGALTextureRange sourceRange{uiSlice, 1, uiSourceMip, 1};
      WGALRenderTargetRange targetRange{uiSlice, 1, uiTargetMip};

      auto pass = ref_renderGraph.AddGraphicsPass("GenerateMipMaps");
      pass.ReadTexture(hGraphTexture, sourceRange, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.AddColorTarget(hGraphTexture, targetRange, {}, {}, WGALResourceFormat::Invalid, WGALTextureType::Texture2DArray);
      pass.HasSideEffects();
      pass.SetExecuteCallback([hGraphTexture, sourceRange](const WRenderGraphContext& context)
        {
          WRenderContext* pRenderContext = context.GetRenderContext();
          pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);
          pRenderContext->BindShader(g_hDownscaleShader);

          WBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
          bindGroup.BindTexture("Input", context.ResolveTexture(hGraphTexture), sourceRange, WGALResourceFormat::Invalid, WGALTextureType::Texture2DArray);

          pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
          pRenderContext->DrawMeshBuffer().IgnoreResult(); });
    }
  }
  ref_renderGraph.PopMarker();
  return hGraphTexture;
}


W_STATICLINK_FILE(RendererCore, RendererCore_RenderGraph_Implementation_RenderGraphUtils);

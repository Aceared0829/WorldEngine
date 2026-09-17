#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReflectionFilterPass, 2, WRTTIDefaultAllocator<WReflectionFilterPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    W_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    W_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    W_MEMBER_PROPERTY("DiffuseIntensity", m_fDiffuseIntensity)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("DiffuseSaturation", m_fDiffuseSaturation)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("SpecularIntensity", m_fSpecularIntensity)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("SpecularOutputIndex", m_uiSpecularOutputIndex),
    W_MEMBER_PROPERTY("IrradianceOutputIndex", m_uiIrradianceOutputIndex),
    W_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WReflectionFilterPass::WReflectionFilterPass()
  : WRenderPipelinePass("ReflectionFilterPass")

{
  {
    m_hFilteredSpecularConstantBuffer = WRenderContext::CreateConstantBufferStorage<WReflectionFilteredSpecularConstants>();
    m_hFilteredSpecularShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.WShader");
    W_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = WRenderContext::CreateConstantBufferStorage<WReflectionIrradianceConstants>();
    m_hIrradianceShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/ReflectionIrradiance.WShader");
    W_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

WReflectionFilterPass::~WReflectionFilterPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hIrradianceConstantBuffer);
}

WStatus WReflectionFilterPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  // Create filtered specular output
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = WReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight = desc.m_uiWidth;
    desc.m_Format = WGALResourceFormat::RGBAHalf;
    desc.m_Type = WGALTextureType::TextureCube;
    desc.m_TextureFlags.Add(WGALTextureUsageFlags::UnorderedAccess);
    desc.m_uiMipLevelCount = WMath::Log2i(desc.m_uiWidth) - 1;
    WRenderGraphTextureHandle hFilteredSpecular = ref_graph.CreateTexture(desc);
    outputs[m_PinFilteredSpecular.m_uiOutputIndex].m_TextureHandle = hFilteredSpecular;
  }

  // Create average luminance output (todo, unused)
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = WGALResourceFormat::RGBAHalf;
    desc.m_Type = WGALTextureType::Texture2D;
    desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::UnorderedAccess);
    desc.m_ResourceAccess.m_bImmutable = false;
    WRenderGraphTextureHandle hAvgLuminance = ref_graph.CreateTexture(desc);
    outputs[m_PinAvgLuminance.m_uiOutputIndex].m_TextureHandle = hAvgLuminance;
  }

  // Create irradiance output
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 6;
    desc.m_uiHeight = 64;
    desc.m_Format = WGALResourceFormat::RGBAHalf;
    desc.m_Type = WGALTextureType::Texture2D;
    desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::UnorderedAccess);
    desc.m_ResourceAccess.m_bImmutable = false;
    WRenderGraphTextureHandle hIrradianceData = ref_graph.CreateTexture(desc);
    outputs[m_PinIrradianceData.m_uiOutputIndex].m_TextureHandle = hIrradianceData;
  }

  WRenderGraphTextureHandle hFilteredSpecular = outputs[m_PinFilteredSpecular.m_uiOutputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hIrradianceData = outputs[m_PinIrradianceData.m_uiOutputIndex].m_TextureHandle;

  // Generate mipmaps
  WRenderGraphTextureHandle hInputCubeTexture;
  auto pInputCubemap = ref_graph.GetDevice()->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
    return W_SUCCESS;
  if (pInputCubemap->GetDescription().m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
  {
    hInputCubeTexture = WRenderGraphUtils::GenerateMipMaps(m_hInputCubemap, {}, ref_graph);
  }
  else
  {
    hInputCubeTexture = ref_graph.ImportTexture(m_hInputCubemap);
  }

  // Filtered specular compute pass
  {
    auto pass = ref_graph.AddComputePass("ReflectionFilterSpecular");
    pass.ReadTexture(hInputCubeTexture, {}, WGALResourceState::ShaderResource);
    pass.WriteTexture(hFilteredSpecular, {}, WGALResourceState::UnorderedAccess);
    pass.HasSideEffects();
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        WGALDevice* pDevice = ctx.GetDevice();

        // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
        WResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
        WResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
        bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
        renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

        W_SCOPE_EXIT(
          renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

        WGALTextureHandle hResolvedSpecular = ctx.ResolveTexture(hFilteredSpecular);
        const WGALTexture* pSpecularTexture = pDevice->GetTexture(hResolvedSpecular);
        if (pSpecularTexture == nullptr)
          return;

        const auto& specDesc = pSpecularTexture->GetDescription();
        WUInt32 uiNumMipMaps = specDesc.m_uiMipLevelCount;
        WUInt32 uiWidth = specDesc.m_uiWidth;
        WUInt32 uiHeight = specDesc.m_uiHeight;

        WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindTexture("InputCubemap", ctx.ResolveTexture(hInputCubeTexture));
        bindGroup.BindBuffer("WReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

        for (WUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
        {
          WGALTextureRange textureRange;
          textureRange.m_uiBaseMipLevel = uiMipMapIndex;
          textureRange.m_uiBaseArraySlice = m_uiSpecularOutputIndex * 6;
          textureRange.m_uiArraySlices = 6;
          bindGroup.BindTexture("ReflectionOutput", hResolvedSpecular, textureRange);
          UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps, uiWidth, uiHeight);

          constexpr WUInt32 uiThreadsX = 8;
          constexpr WUInt32 uiThreadsY = 8;
          const WUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
          const WUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

          renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

          uiWidth >>= 1;
          uiHeight >>= 1;
        } //
      });
  }

  {
    // Irradiance
    auto pass = ref_graph.AddComputePass("Irradiance");
    pass.ReadTexture(hInputCubeTexture, {}, WGALResourceState::ShaderResource);
    pass.WriteTexture(hIrradianceData, {}, WGALResourceState::UnorderedAccess);
    pass.HasSideEffects();
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

        UpdateIrradianceConstantBuffer();
        WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindTexture("IrradianceOutput", ctx.ResolveTexture(hIrradianceData));
        bindGroup.BindTexture("InputCubemap", ctx.ResolveTexture(hInputCubeTexture));
        bindGroup.BindBuffer("WReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

        renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult(); //
      });
  }

  return W_SUCCESS;
}

WResult WReflectionFilterPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fDiffuseIntensity;
  inout_stream << m_fDiffuseSaturation;
  inout_stream << m_fSpecularIntensity;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_hInputCubemap; Runtime only property
  return W_SUCCESS;
}

WResult WReflectionFilterPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  inout_stream >> m_fDiffuseIntensity;
  inout_stream >> m_fDiffuseSaturation;
  if (uiVersion >= 2)
  {
    inout_stream >> m_fSpecularIntensity;
  }
  inout_stream >> m_uiSpecularOutputIndex;
  inout_stream >> m_uiIrradianceOutputIndex;
  return W_SUCCESS;
}

WUInt32 WReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void WReflectionFilterPass::SetInputCubemap(WUInt32 uiCubemapHandle)
{
  WGALTextureHandle hNewCubemapHandle = WGALTextureHandle(WGAL::ez18_14Id(uiCubemapHandle));
  if (m_hInputCubemap != hNewCubemapHandle)
  {
    m_hInputCubemap = WGALTextureHandle(WGAL::ez18_14Id(uiCubemapHandle));
  }
}

void WReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(WUInt32 uiMipMapIndex, WUInt32 uiNumMipMaps, WUInt32 uiWidth, WUInt32 uiHeight)
{
  auto constants = WRenderContext::GetConstantBufferData<WReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel = uiMipMapIndex;
  constants->OutputWidth = uiWidth;
  constants->OutputHeight = uiHeight;
  constants->Intensity = m_fSpecularIntensity;
}

void WReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = WRenderContext::GetConstantBufferData<WReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fDiffuseIntensity;
  constants->Saturation = m_fDiffuseSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);

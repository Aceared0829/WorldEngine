#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/AOPass.h>

#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAOPass, 1, WRTTIDefaultAllocator<WAOPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 10.0f)),
    W_MEMBER_PROPERTY("MaxScreenSpaceRadius", m_fMaxScreenSpaceRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 2.0f)),
    W_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new WDefaultValueAttribute(2.0f)),
    W_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new WDefaultValueAttribute(0.7f)),
    W_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new WDefaultValueAttribute(80.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("FadeOutEnd", GetFadeOutEnd, SetFadeOutEnd)->AddAttributes(new WDefaultValueAttribute(100.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("PositionBias", m_fPositionBias)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("MipLevelScale", m_fMipLevelScale)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("DepthBlurThreshold", m_fDepthBlurThreshold)->AddAttributes(new WDefaultValueAttribute(2.0f), new WClampValueAttribute(0.01f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Post Processing")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAOPass::WAOPass()
  : WRenderPipelinePass("AOPass", true)

{
  m_hNoiseTexture = WResourceManager::LoadResource<WTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/DownscaleDepth.WShader");
  W_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/SSAO.WShader");
  W_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/SSAOBlur.WShader");
  W_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = WRenderContext::CreateConstantBufferStorage<WDownscaleDepthConstants>();
  m_hSSAOConstantBuffer = WRenderContext::CreateConstantBufferStorage<WSSAOConstants>();
}

WAOPass::~WAOPass()
{
  WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);

  WRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  WRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
}

WStatus WAOPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  // Validate input
  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("DepthInput pin: Not connected "));

  WGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepthInput);
  if (depthDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("DepthInput pin: Input must be resolved"));
  // #TODO_RG CHECK IS DEPTH
  //  Create output
  WGALTextureCreationDescription outputDesc = depthDesc;
  outputDesc.m_Format = WGALResourceFormat::RGHalf;

  WRenderGraphTextureHandle hSSAOOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hSSAOOutput;

  // Add passes
  WUInt32 uiWidth = depthDesc.m_uiWidth;
  WUInt32 uiHeight = depthDesc.m_uiHeight;

  WUInt32 uiNumMips = 3;
  WUInt32 uiHzbWidth = WMath::RoundUp(uiWidth, 1u << uiNumMips);
  WUInt32 uiHzbHeight = WMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  {
    {
      WGALTextureCreationDescription desc;
      desc.m_uiWidth = uiHzbWidth / 2;
      desc.m_uiHeight = uiHzbHeight / 2;
      desc.m_uiMipLevelCount = 3;
      desc.m_Type = WGALTextureType::Texture2DArray;
      desc.m_Format = WGALResourceFormat::RHalf;
      desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::ShaderResource);
      desc.m_uiArraySize = outputDesc.m_uiArraySize;

      m_hHzbTexture = ref_graph.CreateTexture(desc);
    }

    for (WUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      m_HzbSizes.PushBack(WVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        WGALTextureRange desc;
        desc.m_uiBaseMipLevel = i;
        desc.m_uiMipLevels = 1;
        desc.m_uiArraySlices = outputDesc.m_uiArraySize;
        m_HzbResourceViews.PushBack(desc);
      }
    }

    WGALTextureCreationDescription descTemp;
    descTemp.SetAsRenderTarget(uiWidth, uiHeight, WGALResourceFormat::RGHalf, WGALMSAASampleCount::None);
    descTemp.m_Type = WGALTextureType::Texture2DArray;
    descTemp.m_uiArraySize = outputDesc.m_uiArraySize;
    m_hSSAOTemp = ref_graph.CreateTexture(descTemp);
  }

  // Mip map passes
  {
    CreateSamplerState();

    for (WUInt32 i = 0; i < uiNumMips; ++i)
    {
      WRenderGraphTextureHandle hInputView = i == 0 ? hDepthInput : m_hHzbTexture;
      WVec2 pixelSize;
      WGALTextureRange range;

      if (i == 0)
      {
        pixelSize = WVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        range = m_HzbResourceViews[i - 1];
        pixelSize = WVec2(1.0f).CompDiv(m_HzbSizes[i - 1]);
      }

      auto pass = ref_graph.AddGraphicsPass("DownscaleDepth");
      pass.AddColorTarget(m_hHzbTexture, WGALRenderTargetRange::MakeFromMipLevel(i));
      pass.ReadTexture(hInputView, range, i == 0 ? WGALResourceState::DepthStencilRead : WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
        {
          const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
          WDownscaleDepthConstants* constants = WRenderContext::GetConstantBufferData<WDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
          constants->PixelSize = pixelSize;
          constants->FadeOutEnd = m_fFadeOutEnd;
          constants->LinearizeDepth = (i == 0);

          WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
          bindGroup.BindBuffer("WDownscaleDepthConstants", m_hDownscaleConstantBuffer);
          renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

          bindGroup.BindTexture("DepthTexture", ctx.ResolveTexture(hInputView), range);
          bindGroup.BindSampler("DepthSampler", m_hSSAOSamplerState);

          renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); //
        });
    }
  }

  // SSAO pass
  {
    auto pass = ref_graph.AddGraphicsPass("SSAO");
    pass.AddColorTarget(m_hSSAOTemp);
    pass.ReadTexture(m_hHzbTexture, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepthInput, {}, WGALResourceState::DepthStencilRead, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        // Update constants
        {
          float fadeOutScale = -1.0f / WMath::Max(0.001f, (m_fFadeOutEnd - m_fFadeOutStart));
          float fadeOutOffset = -fadeOutScale * m_fFadeOutStart + 1.0f;

          WSSAOConstants* constants = WRenderContext::GetConstantBufferData<WSSAOConstants>(m_hSSAOConstantBuffer);
          constants->TexCoordsScale = WVec2(fHzbScaleX, fHzbScaleY);
          constants->FadeOutParams = WVec2(fadeOutScale, fadeOutOffset);
          constants->WorldRadius = m_fRadius;
          constants->MaxScreenSpaceRadius = m_fMaxScreenSpaceRadius;
          constants->Contrast = m_fContrast;
          constants->Intensity = m_fIntensity;
          constants->PositionBias = m_fPositionBias / 1000.0f;
          constants->MipLevelScale = m_fMipLevelScale;
          constants->DepthBlurScale = 1.0f / m_fDepthBlurThreshold;
          constants->FadeOutEnd = m_fFadeOutEnd;
        }

        renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);
        WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
        bindGroupRenderPass.BindBuffer("WSSAOConstants", m_hSSAOConstantBuffer);
        bindGroupRenderPass.BindTexture("DepthTexture", ctx.ResolveTexture(hDepthInput));
        bindGroupRenderPass.BindTexture("LowResDepthTexture", ctx.ResolveTexture(m_hHzbTexture));
        bindGroupRenderPass.BindSampler("DepthSampler", m_hSSAOSamplerState);
        bindGroupRenderPass.BindTexture("NoiseTexture", m_hNoiseTexture, WResourceAcquireMode::BlockTillLoaded);

        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); //
      });
  }

  // Blur pass
  {
    auto pass = ref_graph.AddGraphicsPass("SSAO Blur");
    pass.AddColorTarget(hSSAOOutput);
    pass.ReadTexture(m_hSSAOTemp, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

      WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);
      bindGroupRenderPass.BindBuffer("WSSAOConstants", m_hSSAOConstantBuffer);
      bindGroupRenderPass.BindTexture("SSAOTexture", ctx.ResolveTexture(m_hSSAOTemp));

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return W_SUCCESS;
}

WStatus WAOPass::AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("DepthInput pin: Not connected "));

  WGALTextureCreationDescription outputDesc = ref_graph.GetTextureDesc(hDepthInput);
  outputDesc.m_Format = WGALResourceFormat::RGHalf;

  WRenderGraphTextureHandle hSSAOOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hSSAOOutput;

  auto pass = ref_graph.AddGraphicsPass("InactiveSSAO");
  pass.AddColorTarget(hSSAOOutput, {}, WGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, WColor::White);
  return W_SUCCESS;
}

WResult WAOPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fMaxScreenSpaceRadius;
  inout_stream << m_fContrast;
  inout_stream << m_fIntensity;
  inout_stream << m_fFadeOutStart;
  inout_stream << m_fFadeOutEnd;
  inout_stream << m_fPositionBias;
  inout_stream << m_fMipLevelScale;
  inout_stream << m_fDepthBlurThreshold;
  return W_SUCCESS;
}

WResult WAOPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fMaxScreenSpaceRadius;
  inout_stream >> m_fContrast;
  inout_stream >> m_fIntensity;
  inout_stream >> m_fFadeOutStart;
  inout_stream >> m_fFadeOutEnd;
  inout_stream >> m_fPositionBias;
  inout_stream >> m_fMipLevelScale;
  inout_stream >> m_fDepthBlurThreshold;
  return W_SUCCESS;
}

void WAOPass::SetFadeOutStart(float fStart)
{
  m_fFadeOutStart = WMath::Clamp(fStart, 0.0f, m_fFadeOutEnd);
}

float WAOPass::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void WAOPass::SetFadeOutEnd(float fEnd)
{
  if (m_fFadeOutEnd == fEnd)
    return;

  m_fFadeOutEnd = WMath::Max(fEnd, m_fFadeOutStart);

  WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
}

float WAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void WAOPass::CreateSamplerState()
{
  if (m_hSSAOSamplerState.IsInvalidated())
  {
    WGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = WGALTextureFilterMode::Point;
    desc.m_MagFilter = WGALTextureFilterMode::Point;
    desc.m_MipFilter = WGALTextureFilterMode::Point;
    desc.m_AddressU = WImageAddressMode::ClampBorder;
    desc.m_AddressV = WImageAddressMode::ClampBorder;
    desc.m_AddressW = WImageAddressMode::ClampBorder;
    desc.m_BorderColor = WColor::White * m_fFadeOutEnd;

    m_hSSAOSamplerState = WGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AOPass);

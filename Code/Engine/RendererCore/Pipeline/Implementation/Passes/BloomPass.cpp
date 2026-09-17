#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBloomPass, 1, WRTTIDefaultAllocator<WBloomPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.01f, 1.0f)),
    W_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new WDefaultValueAttribute(0.3f)),
    W_MEMBER_PROPERTY("InnerTintColor", m_InnerTintColor),
    W_MEMBER_PROPERTY("MidTintColor", m_MidTintColor),
    W_MEMBER_PROPERTY("OuterTintColor", m_OuterTintColor),
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

WBloomPass::WBloomPass()
  : WRenderPipelinePass("BloomPass", true)
{
  // Load shader.
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Bloom.WShader");
  W_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");

  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WBloomConstants>();

  const WGALDeviceCapabilities& caps = WGALDevice::GetDefaultDevice()->GetCapabilities();
  const bool bSupportsRG11B10Float = caps.m_FormatSupport[WGALResourceFormat::RG11B10Float].AreAllSet(WGALResourceFormatSupport::RenderTarget | WGALResourceFormatSupport::Texture);
  m_TextureFormat = bSupportsRG11B10Float ? WGALResourceFormat::RG11B10Float : WGALResourceFormat::RGBAHalf;
}

WBloomPass::~WBloomPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

WStatus WBloomPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  // Validate input
  WRenderGraphTextureHandle hColorInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hColorInput);

  // Create output (half-res)
  WGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_uiWidth = outputDesc.m_uiWidth / 2;
  outputDesc.m_uiHeight = outputDesc.m_uiHeight / 2;
  outputDesc.m_Format = m_TextureFormat;

  WRenderGraphTextureHandle hColorOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hColorOutput;

  // Add passes
  WUInt32 uiWidth = inputDesc.m_uiWidth;
  WUInt32 uiHeight = inputDesc.m_uiHeight;
  bool bFastDownscale = WMath::IsEven(uiWidth) && WMath::IsEven(uiHeight);

  const float fMaxRes = (float)WMath::Max(uiWidth, uiHeight);
  const float fRadius = WMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float fDownscaledSize = 4.0f / fRadius;
  const float fNumBlurPasses = WMath::Log2(fMaxRes / fDownscaledSize);
  const WUInt32 uiNumBlurPasses = (WUInt32)WMath::Ceil(fNumBlurPasses);

  // Create temp textures
  WTempHybridArray<WVec2, 8> targetSizes;
  WTempHybridArray<WRenderGraphTextureHandle, 8> tempDownscaleTextures;
  WTempHybridArray<WRenderGraphTextureHandle, 8> tempUpscaleTextures;

  for (WUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth = WMath::Max(uiWidth / 2, 1u);
    uiHeight = WMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(WVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = outputDesc.m_uiArraySize;

    WGALTextureCreationDescription descTemp;
    descTemp.SetAsRenderTarget(uiWidth, uiHeight, uiSliceCount, m_TextureFormat, WGALMSAASampleCount::None);
    tempDownscaleTextures.PushBack(ref_graph.CreateTexture(descTemp));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(ref_graph.CreateTexture(descTemp));
    }
    else
    {
      tempUpscaleTextures.PushBack(WRenderGraphTextureHandle());
    }
  }

  // Downscale passes
  {
    WTempHashedString sInitialDownscale = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    WTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    WTempHashedString sDownscale = "BLOOM_PASS_MODE_DOWNSCALE";
    WTempHashedString sDownscaleFast = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (WUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      WRenderGraphTextureHandle hInput;
      WTempHashedString sPassMode;
      if (i == 0)
      {
        hInput = hColorInput;
        sPassMode = bFastDownscale ? sInitialDownscaleFast : sInitialDownscale;
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        sPassMode = bFastDownscale ? sDownscaleFast : sDownscale;
      }

      WRenderGraphTextureHandle hOutput = tempDownscaleTextures[i];
      WVec2 targetSize = targetSizes[i];
      WColor tintColor = (i == uiNumBlurPasses - 1) ? WColor(m_OuterTintColor) : WColor::White;

      auto pass = ref_graph.AddGraphicsPass("BloomDownscale");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
        {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

        UpdateConstantBuffer(WVec2(1.0f).CompDiv(targetSize), tintColor);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", sPassMode);

        WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindBuffer("WBloomConstants", m_hConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

      bFastDownscale = WMath::IsEven((WInt32)targetSize.x) && WMath::IsEven((WInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass = (uiNumBlurPasses - 1.0f) / 2.0f;

    for (WUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      WRenderGraphTextureHandle hNextInput = tempDownscaleTextures[i];
      WRenderGraphTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      WRenderGraphTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = hColorOutput;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      WVec2 targetSize = targetSizes[i];

      WColor tintColor;
      float fPass = (float)i;
      if (fPass < fMidPass)
      {
        tintColor = WMath::Lerp<WColor>(m_InnerTintColor, m_MidTintColor, fPass / fMidPass);
      }
      else
      {
        tintColor = WMath::Lerp<WColor>(m_MidTintColor, m_OuterTintColor, (fPass - fMidPass) / fMidPass);
      }

      auto pass = ref_graph.AddGraphicsPass("BloomUpscale");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.ReadTexture(hNextInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
        {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

        UpdateConstantBuffer(WVec2(fBlurRadius).CompDiv(targetSize), tintColor);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

        WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindBuffer("WBloomConstants", m_hConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        bindGroup.BindTexture("NextColorTexture", ctx.ResolveTexture(hNextInput));
        bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
    }
  }

  return W_SUCCESS;
}

WStatus WBloomPass::AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hColorInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  WGALTextureCreationDescription outputDesc = ref_graph.GetTextureDesc(hColorInput);
  outputDesc.m_uiWidth = outputDesc.m_uiWidth / 2;
  outputDesc.m_uiHeight = outputDesc.m_uiHeight / 2;
  outputDesc.m_Format = m_TextureFormat;

  WRenderGraphTextureHandle hColorOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hColorOutput;

  auto pass = ref_graph.AddGraphicsPass("InactiveBloom");
  pass.AddColorTarget(hColorOutput, {}, WGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, WColor::Black);
  return W_SUCCESS;
}

WResult WBloomPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fThreshold;
  inout_stream << m_fIntensity;
  inout_stream << m_InnerTintColor;
  inout_stream << m_MidTintColor;
  inout_stream << m_OuterTintColor;
  return W_SUCCESS;
}

WResult WBloomPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fThreshold;
  inout_stream >> m_fIntensity;
  inout_stream >> m_InnerTintColor;
  inout_stream >> m_MidTintColor;
  inout_stream >> m_OuterTintColor;
  return W_SUCCESS;
}

void WBloomPass::UpdateConstantBuffer(WVec2 pixelSize, const WColor& tintColor)
{
  WBloomConstants* constants = WRenderContext::GetConstantBufferData<WBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BloomPass);

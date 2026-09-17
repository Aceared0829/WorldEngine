#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Pipeline/Passes/LightShaftsPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Shaders/Pipeline/LightShaftsConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLightShaftsPass, 1, WRTTIDefaultAllocator<WLightShaftsPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    W_ACCESSOR_PROPERTY("DownsampleFactor", GetDownsampleFactor, SetDownsampleFactor)->AddAttributes(new WDefaultValueAttribute(3), new WClampValueAttribute(1, 8)),
    W_ACCESSOR_PROPERTY("NumBlurPasses", GetNumBlurPasses, SetNumBlurPasses)->AddAttributes(new WDefaultValueAttribute(3), new WClampValueAttribute(1, 5)),
    W_ACCESSOR_PROPERTY("NumSamples", GetNumSamples, SetNumSamples)->AddAttributes(new WDefaultValueAttribute(12), new WClampValueAttribute(8, 16)),
    W_ACCESSOR_PROPERTY("MaxBlurDistance", GetMaxBlurDistance, SetMaxBlurDistance)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f))
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

WLightShaftsPass::WLightShaftsPass()
  : WRenderPipelinePass("LightShaftsPass", true)
{
  m_hMaskShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LightShaftsMask.WShader");
  m_hRadialBlurShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LightShaftsRadialBlur.WShader");
  m_hApplyShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LightShaftsApply.WShader");

  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WLightShaftsConstants>();

  const WGALDeviceCapabilities& caps = WGALDevice::GetDefaultDevice()->GetCapabilities();
  const bool bSupportsRG11B10Float = caps.m_FormatSupport[WGALResourceFormat::RG11B10Float].AreAllSet(WGALResourceFormatSupport::RenderTarget | WGALResourceFormatSupport::Texture);
  m_TextureFormat = bSupportsRG11B10Float ? WGALResourceFormat::RG11B10Float : WGALResourceFormat::RGBAHalf;
}

WLightShaftsPass::~WLightShaftsPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

WStatus WLightShaftsPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hColorInput = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return WStatus(WFmt("Color: Not connected"));

  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("DepthInput: Not connected"));

  const WGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepthInput);
  if (depthDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("DepthInput: Must be resolved (non-MSAA)"));

  const WGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColorInput);
  if (colorDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("Color: Must be resolved (non-MSAA)"));

  // Pass-through color
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColorInput;

  // Compute light origin UVs from camera and light direction
  auto pClusteredData = GetPipeline()->GetRenderData().GetFrameData<WClusteredDataCPU>();
  if (pClusteredData == nullptr || pClusteredData->m_vLightShaftsDirection.IsZero())
    return W_SUCCESS;

  // Note these computations here make any graph using this pass uncachable as the logic depends on the current camera.
  const WVec4 vLightOriginUVs = CalculateOriginUVs(pClusteredData->m_vLightShaftsDirection, viewData, camera);
  if (vLightOriginUVs.w < 0.0f)
    return W_SUCCESS;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // Setup downsampled dimensions
  WUInt32 uiDownsampledWidth = WMath::Max((colorDesc.m_uiWidth + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  WUInt32 uiDownsampledHeight = WMath::Max((colorDesc.m_uiHeight + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  WUInt32 uiSliceCount = colorDesc.m_uiArraySize;
  WRectFloat downsampledRect((float)uiDownsampledWidth, (float)uiDownsampledHeight);

  // Create temp textures
  WGALTextureCreationDescription tempDesc;
  tempDesc.SetAsRenderTarget(uiDownsampledWidth, uiDownsampledHeight, uiSliceCount, m_TextureFormat, WGALMSAASampleCount::None);

  WRenderGraphTextureHandle hTempTextures[2];
  hTempTextures[0] = ref_graph.CreateTexture(tempDesc);
  hTempTextures[1] = ref_graph.CreateTexture(tempDesc);

  WUInt32 uiCurrentInputTempTexture = 0;

  // Pass 1: Generate mask at downsampled resolution
  {
    auto pass = ref_graph.AddGraphicsPass("LightShafts Mask");
    pass.AddColorTarget(hTempTextures[0]);
    pass.ReadTexture(hColorInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepthInput, {}, WGALResourceState::DepthStencilRead, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

      UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), 0.0f);

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      bindGroup.BindBuffer("WLightShaftsConstants", m_hConstantBuffer);
      bindGroup.BindTexture("SceneColor", ctx.ResolveTexture(hColorInput));
      bindGroup.BindTexture("SceneDepth", ctx.ResolveTexture(hDepthInput));

      renderViewContext.m_pRenderContext->BindShader(m_hMaskShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // Pass 2: Radial blur passes
  {
    const float fFirstPassBlurDistance = m_fMaxBlurDistance / static_cast<float>(1 << (m_uiNumBlurPasses - 1));

    for (WUInt32 uiPassIndex = 0; uiPassIndex < m_uiNumBlurPasses; ++uiPassIndex)
    {
      const float fBlurDistance = WMath::Min((1 << uiPassIndex) * fFirstPassBlurDistance, 1.0f);
      const float fBlurStep = fBlurDistance / static_cast<float>(m_uiNumSamples);

      const WUInt32 uiCurrentOutputTempTexture = (uiCurrentInputTempTexture + 1) % 2;

      WRenderGraphTextureHandle hInput = hTempTextures[uiCurrentInputTempTexture];
      WRenderGraphTextureHandle hOutput = hTempTextures[uiCurrentOutputTempTexture];

      auto pass = ref_graph.AddGraphicsPass("LightShafts RadialBlur");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
        {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), fBlurStep);
        WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
        bindGroup.BindBuffer("WLightShaftsConstants", m_hConstantBuffer);
        bindGroup.BindTexture("LightShaftsTexture", ctx.ResolveTexture(hInput));

        renderViewContext.m_pRenderContext->BindShader(m_hRadialBlurShader);
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

      uiCurrentInputTempTexture = uiCurrentOutputTempTexture;
    }
  }

  // Pass 3: Apply to scene color at full resolution
  {
    WRenderGraphTextureHandle hBlurResult = hTempTextures[uiCurrentInputTempTexture];

    auto pass = ref_graph.AddGraphicsPass("LightShafts Apply");
    pass.AddColorTarget(hColorInput);
    pass.ReadTexture(hBlurResult, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      bindGroup.BindBuffer("WLightShaftsConstants", m_hConstantBuffer);
      bindGroup.BindTexture("LightShaftsTexture", ctx.ResolveTexture(hBlurResult));

      renderViewContext.m_pRenderContext->BindShader(m_hApplyShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return W_SUCCESS;
}

WResult WLightShaftsPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiDownsampleFactor;
  inout_stream << m_uiNumBlurPasses;
  inout_stream << m_uiNumSamples;
  inout_stream << m_fMaxBlurDistance;
  return W_SUCCESS;
}

WResult WLightShaftsPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  inout_stream >> m_uiDownsampleFactor;
  inout_stream >> m_uiNumBlurPasses;
  inout_stream >> m_uiNumSamples;
  inout_stream >> m_fMaxBlurDistance;
  return W_SUCCESS;
}

void WLightShaftsPass::SetDownsampleFactor(WUInt32 uiFactor)
{
  m_uiDownsampleFactor = WMath::Clamp(uiFactor, 1u, 8u);
}

void WLightShaftsPass::SetNumBlurPasses(WUInt32 uiPasses)
{
  m_uiNumBlurPasses = WMath::Clamp(uiPasses, 1u, 5u);
}

void WLightShaftsPass::SetNumSamples(WUInt32 uiSamples)
{
  m_uiNumSamples = WMath::Clamp(uiSamples, 8u, 16u);
}

void WLightShaftsPass::SetMaxBlurDistance(float fDistance)
{
  m_fMaxBlurDistance = WMath::Saturate(fDistance);
}

WVec4 WLightShaftsPass::CalculateOriginUVs(const WVec3& vLightDirection, const WViewData& viewData, const WCamera& camera) const
{
  const WVec3 vLightPos = vLightDirection * (camera.GetFarPlane() * 0.999f) + camera.GetCenterPosition();

  const WVec4 vLightPosClipSpace = viewData.m_ViewProjectionMatrix[0] * vLightPos.GetAsPositionVec4();
  const float fInvW = 1.0f / vLightPosClipSpace.w;

  const WVec2 vProjected = vLightPosClipSpace.GetAsVec2() * fInvW;
  const WVec2 vOriginUVs = vProjected.CompMul(WVec2(0.5f, -0.5f)) + WVec2(0.5f);

  return WVec4(vOriginUVs.x, vOriginUVs.y, 0.0f, vLightPosClipSpace.w);
}

void WLightShaftsPass::UpdateConstantBuffer(const WClusteredDataCPU& clusteredData, const WVec2& vLightOriginUVs, float fBlurStep)
{
  WLightShaftsConstants* pConstants = WRenderContext::GetConstantBufferData<WLightShaftsConstants>(m_hConstantBuffer);

  pConstants->LightShaftsTintColor = clusteredData.m_LightShaftsTintColor;

  pConstants->LightShaftsIntensity = clusteredData.m_fLightShaftsIntensity;
  pConstants->LightShaftsMaxBrightness = clusteredData.m_fLightShaftsMaxBrightness;
  pConstants->LightShaftsBrightnessThreshold = clusteredData.m_fLightShaftsBrightnessThreshold;
  pConstants->LightShaftsDiskMaskRadius = clusteredData.m_fLightShaftsDiskMaskRadius;

  pConstants->LightShaftsOriginUVs = vLightOriginUVs;
  pConstants->LightShaftsNumBlurSamples = m_uiNumSamples;
  pConstants->LightShaftsBlurStep = fBlurStep;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LightShaftsPass);

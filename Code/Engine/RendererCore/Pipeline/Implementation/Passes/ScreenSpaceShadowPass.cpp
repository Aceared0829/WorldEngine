#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Pipeline/Passes/ScreenSpaceShadowPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Shaders/Pipeline/ScreenSpaceShadowConstants.h>

W_WARNING_PUSH()
W_WARNING_DISABLE_MSVC(4244)
#include <Shaders/Pipeline/bend_sss/bend_sss_cpu.h>
W_WARNING_POP()

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScreenSpaceShadowPass, 1, WRTTIDefaultAllocator<WScreenSpaceShadowPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_MEMBER_PROPERTY("SurfaceThickness", m_fSurfaceThickness)->AddAttributes(new WDefaultValueAttribute(0.005f), new WClampValueAttribute(0.001f, 0.5f)),
    W_MEMBER_PROPERTY("ShadowContrast", m_fShadowContrast)->AddAttributes(new WDefaultValueAttribute(4.0f), new WClampValueAttribute(1.0f, 10.0f)),
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

WScreenSpaceShadowPass::WScreenSpaceShadowPass()
  : WRenderPipelinePass("ScreenSpaceShadowPass", true)
{
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/ScreenSpaceShadow.WShader");
  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WScreenSpaceShadowConstants>();
}

WScreenSpaceShadowPass::~WScreenSpaceShadowPass()
{
  WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hDepthSamplerState);

  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

WStatus WScreenSpaceShadowPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("DepthInput: Not connected"));

  const WGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepthInput);
  if (depthDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("DepthInput: Must be resolved"));

  // Create output texture
  WGALTextureCreationDescription outputDesc = depthDesc;
  outputDesc.m_Format = WGALResourceFormat::RUByteNormalized;
  outputDesc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::UnorderedAccess;

  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  const WGALTextureCreationDescription& depthTexDesc = ref_graph.GetTextureDesc(hDepthInput);
  const WUInt32 uiWidth = depthTexDesc.m_uiWidth;
  const WUInt32 uiHeight = depthTexDesc.m_uiHeight;

  CreateSamplerState();

  bool bRendered = false;

  auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(WDefaultRenderDataCategories::Light);
  const WUInt32 uiBatchCount = batchList.GetBatchCount();
  for (WUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const WRenderDataBatch& batch = batchList.GetBatch(i);

    for (auto it = batch.GetIterator<WRenderData>(); it.IsValid(); ++it)
    {
      const auto pDirLight = WDynamicCast<const WDirectionalLightRenderData*>(it);
      if (pDirLight == nullptr)
      {
        // Directional lights come first in the list, so we can stop as soon as we encounter a non-directional light.
        return W_SUCCESS;
      }

      if (!pDirLight->m_bScreenSpaceShadows)
        continue;

      if (bRendered)
      {
        WLog::Warning("Multiple directional lights with screen space shadows are not yet supported. Only the first one will be rendered.");
        return W_SUCCESS;
      }

      WVec3 lightDir = pDirLight->m_vDirection;
      lightDir.Normalize();

      const WUInt32 uiEyeCount = camera.IsStereoscopic() ? 2 : 1;
      for (WUInt32 uiEyeIndex = 0; uiEyeIndex < uiEyeCount; ++uiEyeIndex)
      {
        // Ray march pass: writes raw shadow to output texture
        auto pass = ref_graph.AddComputePass("ScreenSpaceShadow");
        pass.ReadTexture(hDepthInput, {}, WGALResourceState::ShaderResource);
        pass.WriteTexture(hOutput, {}, WGALResourceState::UnorderedAccess);
        pass.SetStereoscopic(camera.IsStereoscopic());
        pass.SetExecuteCallback([this, uiWidth, uiHeight, hDepthInput, hOutput, uiEyeIndex, lightDir](const WRenderGraphContext& ctx)
          {
            const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
            WVec2I32 viewportSize = WVec2I32::Make(uiWidth, uiHeight);
            WVec2I32 minRenderBounds = WVec2I32::Make(0, 0);
            WVec2I32 maxRenderBounds = viewportSize;
            WVec4 projectedLightDir = renderViewContext.m_pViewData->m_ViewProjectionMatrix[uiEyeIndex] * WVec4(lightDir, 0.0f);

            Bend::DispatchList dispatchList = Bend::BuildDispatchList(projectedLightDir.GetData(), viewportSize.GetData(), minRenderBounds.GetData(), maxRenderBounds.GetData(), false, WAVE_SIZE);

            WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
            bindGroup.BindBuffer("WScreenSpaceShadowConstants", m_hConstantBuffer);
            bindGroup.BindTexture("DepthTexture", ctx.ResolveTexture(hDepthInput));
            bindGroup.BindSampler("DepthTextureSampler", m_hDepthSamplerState);
            bindGroup.BindTexture("OutputTexture", ctx.ResolveTexture(hOutput));

            renderViewContext.m_pRenderContext->BindShader(m_hShader);

            for (int j = 0; j < dispatchList.DispatchCount; ++j)
            {
              auto& dispatch = dispatchList.Dispatch[j];

              // Fill constant buffer
              {
                auto cb = WRenderContext::GetConstantBufferData<WScreenSpaceShadowConstants>(m_hConstantBuffer);
                cb->LightCoordinate = WVec4(dispatchList.LightCoordinate_Shader[0], dispatchList.LightCoordinate_Shader[1], dispatchList.LightCoordinate_Shader[2], dispatchList.LightCoordinate_Shader[3]);
                cb->WaveOffset = WVec2I32(dispatch.WaveOffset_Shader[0], dispatch.WaveOffset_Shader[1]);
                cb->InvDepthTextureSize = WVec2(1.0f / uiWidth, 1.0f / uiHeight);
                cb->SurfaceThickness = m_fSurfaceThickness;
                cb->ShadowContrast = m_fShadowContrast;
                cb->EyeIndex = uiEyeIndex;
              }

              renderViewContext.m_pRenderContext->Dispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]).IgnoreResult();
            } //
          });
      }

      bRendered = true;
    }
  } //

  return W_SUCCESS;
}

WResult WScreenSpaceShadowPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fSurfaceThickness;
  inout_stream << m_fShadowContrast;
  return W_SUCCESS;
}

WResult WScreenSpaceShadowPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  inout_stream >> m_fSurfaceThickness;
  inout_stream >> m_fShadowContrast;

  return W_SUCCESS;
}

void WScreenSpaceShadowPass::CreateSamplerState()
{
  if (m_hDepthSamplerState.IsInvalidated())
  {
    WGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = WGALTextureFilterMode::Point;
    desc.m_MagFilter = WGALTextureFilterMode::Point;
    desc.m_MipFilter = WGALTextureFilterMode::Point;
    desc.m_AddressU = WImageAddressMode::ClampBorder;
    desc.m_AddressV = WImageAddressMode::ClampBorder;
    desc.m_AddressW = WImageAddressMode::ClampBorder;
    desc.m_BorderColor = WColor::White;

    m_hDepthSamplerState = WGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ScreenSpaceShadowPass);

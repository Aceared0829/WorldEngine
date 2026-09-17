#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTonemapPass, 2, WRTTIDefaultAllocator<WTonemapPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColorInput),
    W_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_RESOURCE_MEMBER_PROPERTY("VignettingTexture", m_hVignettingTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new WDefaultValueAttribute("White.color")),
    W_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new WDefaultValueAttribute(WColor::Orange)),
    W_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new WClampValueAttribute(0.0f, 2.0f), new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_RESOURCE_MEMBER_PROPERTY("LUT1", m_hLUT1)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    W_RESOURCE_MEMBER_PROPERTY("LUT2", m_hLUT2)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    W_MEMBER_PROPERTY("WhitePoint", m_fWhitePoint)->AddAttributes(new WClampValueAttribute(0.0f, 50.0f), new WDefaultValueAttribute(11.2f)),
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

WTonemapPass::WTonemapPass()
  : WRenderPipelinePass("TonemapPass", true)
{
  m_hVignettingTexture = WResourceManager::LoadResource<WTexture2DResource>("White.color");
  m_hNoiseTexture = WResourceManager::LoadResource<WTexture2DResource>("Textures/BlueNoise.dds");
  m_hBlackTexture = WResourceManager::LoadResource<WTexture2DResource>("Black.color");

  m_MoodColor = WColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;
  m_fWhitePoint = 11.2f;

  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Tonemap.WShader");
  W_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WTonemapConstants>();
}

WTonemapPass::~WTonemapPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

WStatus WTonemapPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hColorInput = inputs[m_PinColorInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return WStatus(WFmt("Color input: Not connected"));

  const WGALTextureCreationDescription colorInputDesc = ref_graph.GetTextureDesc(hColorInput);

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  const WGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();
  const WGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
  if (pTexture == nullptr)
    return WStatus(WFmt("View does not have a valid color target"));

  const WGALTextureCreationDescription& rtDesc = pTexture->GetDescription();
  WGALTextureCreationDescription outputDesc;
  outputDesc.SetAsRenderTarget(colorInputDesc.m_uiWidth, colorInputDesc.m_uiHeight, rtDesc.m_Format);
  outputDesc.m_Type = colorInputDesc.m_Type;
  outputDesc.m_uiArraySize = colorInputDesc.m_uiArraySize;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  WRenderGraphTextureHandle hBloomInput = inputs[m_PinBloomInput.m_uiInputIndex].m_TextureHandle;

  auto pass = ref_graph.AddGraphicsPass("Tonemap");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hColorInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  if (!hBloomInput.IsInvalidated())
    pass.ReadTexture(hBloomInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    // Determine how many LUTs are active
    WUInt32 numLUTs = 0;
    WTexture3DResourceHandle luts[2] = {};
    float lutStrengths[2] = {};

    if (m_hLUT1.IsValid())
    {
      luts[numLUTs] = m_hLUT1;
      lutStrengths[numLUTs] = m_fLut1Strength;
      numLUTs++;
    }

    if (m_hLUT2.IsValid())
    {
      luts[numLUTs] = m_hLUT2;
      lutStrengths[numLUTs] = m_fLut2Strength;
      numLUTs++;
    }

    {
      WTonemapConstants* constants = WRenderContext::GetConstantBufferData<WTonemapConstants>(m_hConstantBuffer);
      constants->AutoExposureParams.SetZero();
      constants->MoodColor = m_MoodColor;
      constants->MoodStrength = m_fMoodStrength;
      constants->Saturation = m_fSaturation;
      constants->Lut1Strength = lutStrengths[0];
      constants->Lut2Strength = lutStrengths[1];
      constants->WhitePoint = m_fWhitePoint;

      // Pre-calculate factors of a s-shaped polynomial-function
      const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
      const float a = 2.0f * m - 2.0f;
      const float b = -3.0f * m + 3.0f;

      constants->ContrastParams = WVec4(a, b, m, 0.0f);
    }

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

    WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroup.BindBuffer("WTonemapConstants", m_hConstantBuffer);
    bindGroup.BindTexture("VignettingTexture", m_hVignettingTexture, WResourceAcquireMode::BlockTillLoaded);
    bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture, WResourceAcquireMode::BlockTillLoaded);
    bindGroup.BindTexture("SceneColorTexture", ctx.ResolveTexture(hColorInput));
    bindGroup.BindTexture("Lut1Texture", luts[0]);
    bindGroup.BindTexture("Lut2Texture", luts[1]);

    if (!hBloomInput.IsInvalidated())
    {
      bindGroup.BindTexture("BloomTexture", ctx.ResolveTexture(hBloomInput));
    }
    else
    {
      bindGroup.BindTexture("BloomTexture", m_hBlackTexture, WResourceAcquireMode::BlockTillLoaded);
    }

    WTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return W_SUCCESS;
}

WResult WTonemapPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  WStringBuilder sTemp = GetVignettingTextureFile();
  inout_stream << sTemp;
  inout_stream << m_MoodColor;
  inout_stream << m_fMoodStrength;
  inout_stream << m_fSaturation;
  inout_stream << m_fContrast;
  inout_stream << m_fLut1Strength;
  inout_stream << m_fLut2Strength;
  sTemp = GetLUT1TextureFile();
  inout_stream << sTemp;
  sTemp = GetLUT2TextureFile();
  inout_stream << sTemp;
  inout_stream << m_fWhitePoint;
  return W_SUCCESS;
}

WResult WTonemapPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  WStringBuilder sTemp;
  inout_stream >> sTemp;
  SetVignettingTextureFile(sTemp);
  inout_stream >> m_MoodColor;
  inout_stream >> m_fMoodStrength;
  inout_stream >> m_fSaturation;
  inout_stream >> m_fContrast;
  inout_stream >> m_fLut1Strength;
  inout_stream >> m_fLut2Strength;
  inout_stream >> sTemp;
  SetLUT1TextureFile(sTemp);
  inout_stream >> sTemp;
  SetLUT2TextureFile(sTemp);

  if (uiVersion >= 2)
  {
    inout_stream >> m_fWhitePoint;
  }

  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);

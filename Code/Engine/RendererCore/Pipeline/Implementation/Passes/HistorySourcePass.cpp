#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WHistorySourcePassTextureDataProvider, 1, WRTTIDefaultAllocator<WHistorySourcePassTextureDataProvider>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WHistorySourcePass, 2, WRTTIDefaultAllocator<WHistorySourcePass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_ENUM_MEMBER_PROPERTY("Type", WRequiredTextureType, m_Type),
    W_ENUM_MEMBER_PROPERTY("Precision", WRequiredTexturePrecision, m_MinPrecision),
    W_ENUM_MEMBER_PROPERTY("Channels", WRequiredTextureChannels, m_MinChannels),
    W_ENUM_MEMBER_PROPERTY("MSAA_Mode", WGALMSAASampleCount, m_MsaaMode),
    W_MEMBER_PROPERTY("UAV", m_bUAV),
    W_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_MEMBER_PROPERTY("ClearDepth", m_fClearDepth)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WHistorySourcePassTextureDataProvider::WHistorySourcePassTextureDataProvider() = default;
WHistorySourcePassTextureDataProvider::~WHistorySourcePassTextureDataProvider()
{
  while (!m_Data.IsEmpty())
  {
    ResetTexture(m_Data.GetIterator().Key());
  }
}

void WHistorySourcePassTextureDataProvider::ResetTexture(WStringView sSourcePassName)
{
  if (WGALTextureHandle* pHandle = m_Data.GetValue(sSourcePassName))
  {
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(*pHandle);
    m_Data.Remove(sSourcePassName);
  }
}

WGALTextureHandle WHistorySourcePassTextureDataProvider::GetOrCreateTexture(WStringView sSourcePassName, const WGALTextureCreationDescription& desc)
{
  bool bExisted;
  WGALTextureHandle& hTexture = m_Data.FindOrAdd(sSourcePassName, &bExisted);
  if (!bExisted)
  {
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    hTexture = pDevice->CreateTexture(desc);
    if (hTexture.IsInvalidated())
      WLog::Error("Failed to create history source pass texture");
  }
  return hTexture;
}


WHistorySourcePass::WHistorySourcePass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WHistorySourcePass::~WHistorySourcePass() = default;

WStatus WHistorySourcePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<WHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(GetName());
  WGALTextureCreationDescription desc;
  W_SUCCEED_OR_RETURN(WSourcePass::GetOutputDescription(viewData, camera, m_Type, m_MinPrecision, m_MinChannels, m_MsaaMode, m_bUAV, desc));
  WGALTextureHandle hTexture = QueryTextureProvider(&m_PinOutput, desc);
  WRenderGraphTextureHandle hGraphTexture = ref_graph.ImportTexture(hTexture);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hGraphTexture;

  // When we encounter the texture for the first time, the history target has never been written before and we clear it.
  if (hTexture != m_hTextureCleared)
  {
    m_hTextureCleared = hTexture;
    auto pass = ref_graph.AddGraphicsPass(GetName());
    if (WGALResourceFormat::IsDepthFormat(desc.m_Format))
    {
      pass.AddDepthStencilTarget(hGraphTexture, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearDepth().SetClearStencil();
      pass.SetClearDepth(m_fClearDepth);
    }
    else
    {
      pass.AddDepthStencilTarget(hGraphTexture, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, m_ClearColor);
    }
  }

  return W_SUCCESS;
}

WGALTextureHandle WHistorySourcePass::QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<WHistorySourcePassTextureDataProvider>();
  return pData->GetOrCreateTexture(GetName(), desc);
}

WResult WHistorySourcePass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Type;
  inout_stream << m_MinPrecision;
  inout_stream << m_MinChannels;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_fClearDepth;
  inout_stream << m_bUAV;
  return W_SUCCESS;
}

WResult WHistorySourcePass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  if (uiVersion >= 2)
  {
    inout_stream >> m_Type;
    inout_stream >> m_MinPrecision;
    inout_stream >> m_MinChannels;
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_fClearDepth;
    inout_stream >> m_bUAV;
  }
  else
  {
    // Version 1 stored the removed WSourceFormat enum, which used WUInt8 as storage.
    WUInt8 uiLegacyFormat = 0;
    inout_stream >> uiLegacyFormat;
    WGetLegacyTextureFormatRequirements(uiLegacyFormat, false, m_Type, m_MinPrecision, m_MinChannels);
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
  }

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// Replaces the removed WSourceFormat 'Format' property with the texture requirement properties.
class WHistorySourcePassPatch_1_2 : public WGraphPatch
{
public:
  WHistorySourcePassPatch_1_2()
    : WGraphPatch("WHistorySourcePass", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    WPatchLegacyTextureFormatProperty(pNode, false);
  }
};

WHistorySourcePassPatch_1_2 g_WHistorySourcePassPatch_1_2;


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_HistorySourcePass);

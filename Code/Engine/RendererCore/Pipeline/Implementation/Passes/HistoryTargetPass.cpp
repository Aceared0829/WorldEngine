#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/HistoryTargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WHistoryTargetPass, 1, WRTTIDefaultAllocator<WHistoryTargetPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new WDefaultValueAttribute("HistorySourcePass"))
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Output")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WHistoryTargetPass::WHistoryTargetPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WHistoryTargetPass::~WHistoryTargetPass() = default;

WStatus WHistoryTargetPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<WHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(m_sSourcePassName);
  return W_SUCCESS;
}

WGALTextureHandle WHistoryTargetPass::QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<WHistorySourcePassTextureDataProvider>();
  return pData->GetOrCreateTexture(m_sSourcePassName, desc);
}

WResult WHistoryTargetPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sSourcePassName;
  return W_SUCCESS;
}

WResult WHistoryTargetPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sSourcePassName;
  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_HistoryTargetPass);

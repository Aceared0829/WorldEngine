#include <RendererCore/RendererCorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RendererRegistry.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(WRenderPipelinePass, 1)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Grape))
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WForwardRenderShadingQuality, 1)
  W_ENUM_CONSTANTS(WForwardRenderShadingQuality::Normal, WForwardRenderShadingQuality::Simplified)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WRenderPipelinePass::WRenderPipelinePass(const char* szName, bool bIsStereoAware)
  : m_bIsStereoAware(bIsStereoAware)

{
  m_sName.Assign(szName);
}

WRenderPipelinePass::~WRenderPipelinePass() = default;

void WRenderPipelinePass::SetName(const char* szName)
{
  if (!WStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* WRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void WRenderPipelinePass::ReadBackProperties(WView* pView) {}

WResult WRenderPipelinePass::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return W_SUCCESS;
}

WResult WRenderPipelinePass::Deserialize(WStreamReader& inout_stream)
{
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return W_SUCCESS;
}

void WRenderPipelinePass::DeclareRendererDependenciesForCategory(WRenderData::Category category, WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_passBuilder)
{
  for (const WTextureDependency& dep : m_pPipeline->GetTextureDependenciesWithCategory(category))
  {
    ref_passBuilder.ReadTexture(ref_graph.ImportTexture(dep.m_hTexture), {}, dep.m_RequiredState, dep.m_Stage);
  }

  for (const WBufferDependency& dep : m_pPipeline->GetBufferDependenciesWithCategory(category))
  {
    ref_passBuilder.ReadBuffer(ref_graph.ImportBuffer(dep.m_hBuffer), dep.m_RequiredState, dep.m_Stage);
  }
}

void WRenderPipelinePass::RenderDataWithCategory(const WRenderViewContext& renderViewContext, WRenderData::Category category)
{
  W_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), WRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category);
  const WUInt32 uiBatchCount = batchList.GetBatchCount();
  for (WUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const WRenderDataBatch& batch = batchList.GetBatch(i);

    if (const WRenderData* pRenderData = batch.GetFirstData<WRenderData>())
    {
      const WRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const WRenderer* pRenderer = WRendererRegistry::GetRenderer(pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}

void WRenderPipelinePass::SetReadBackProperty(WView* pView, WStringView sPropertyName, const WVariant& value)
{
  WStringBuilder sb = GetName();
  sb.Append(".", sPropertyName);

  pView->GetBlackboard()->SetEntryValue(sb, value);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);

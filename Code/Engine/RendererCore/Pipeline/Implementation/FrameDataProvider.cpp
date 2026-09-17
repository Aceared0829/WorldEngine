#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFrameDataProviderBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WFrameDataProviderBase::WFrameDataProviderBase()

  = default;

void* WFrameDataProviderBase::GetData(const WRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != WRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = WRenderWorld::GetFrameCounter();
  }

  return m_pData;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);

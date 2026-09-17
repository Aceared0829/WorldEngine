#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTargetPass, 1, WRTTIDefaultAllocator<WTargetPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color0", m_PinColor0),
    W_MEMBER_PROPERTY("Color1", m_PinColor1),
    W_MEMBER_PROPERTY("Color2", m_PinColor2),
    W_MEMBER_PROPERTY("Color3", m_PinColor3),
    W_MEMBER_PROPERTY("Color4", m_PinColor4),
    W_MEMBER_PROPERTY("Color5", m_PinColor5),
    W_MEMBER_PROPERTY("Color6", m_PinColor6),
    W_MEMBER_PROPERTY("Color7", m_PinColor7),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
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

WTargetPass::WTargetPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WTargetPass::~WTargetPass() = default;

WStatus WTargetPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  m_hSwapChain = viewData.m_hSwapChain;
  m_RenderTargets = viewData.m_RenderTargets;

  WTempHashedString pinNames[] = {
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "Color4",
    "Color5",
    "Color6",
    "Color7",
    "DepthStencil",
  };

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(pinNames); ++i)
  {
    W_SUCCEED_OR_RETURN(VerifyInput(ref_graph, inputs, pinNames[i]));
  }

  return W_SUCCESS;
}

WGALTextureHandle WTargetPass::QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc)
{
  W_ASSERT_DEV(pPin->m_pParent == this, "WTargetPass::QueryTextureProvider: The given pin is not part of this pass!");

  auto GetActiveRenderTargets = [&]() -> const WGALRenderTargets&
  {
    if (const WGALSwapChain* pSwapChain = WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain))
    {
      return pSwapChain->GetRenderTargets();
    }
    return m_RenderTargets;
  };
  const WGALRenderTargets& renderTargets = GetActiveRenderTargets();

  WGALTextureHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    return renderTargets.m_hDSTarget;
  }
  else
  {
    return renderTargets.m_hRTs[pPin->m_uiInputIndex];
  }
}

WStatus WTargetPass::VerifyInput(WRenderGraph& graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WTempHashedString sPinName)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  const WRenderPipelineNodePin* pPin = GetPinByName(sPinName);
  if (inputs[pPin->m_uiInputIndex].m_Connectivity == WRenderPipelinePinConnection::Connectivity::Texture)
  {
    const WGALTextureCreationDescription& desc = graph.GetTextureDesc(inputs[pPin->m_uiInputIndex].m_TextureHandle);
    const WGALTextureHandle handle = QueryTextureProvider(pPin, desc);
    if (!handle.IsInvalidated())
    {
      const WGALTexture* pTexture = pDevice->GetTexture(handle);
      if (pTexture)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return W_SUCCESS;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TargetPass);

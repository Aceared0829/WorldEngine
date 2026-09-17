#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WGALSwapChain, WNoBase, 1, WRTTINoAllocator)
{
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WGALWindowSwapChain, WGALSwapChain, 1, WRTTINoAllocator)
{
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WGALSwapChainCreationDescription CreateSwapChainCreationDescription(const WRTTI* pType)
{
  WGALSwapChainCreationDescription desc;
  desc.m_pSwapChainType = pType;
  return desc;
}

WGALSwapChain::WGALSwapChain(const WRTTI* pSwapChainType)
  : WGALObject(CreateSwapChainCreationDescription(pSwapChainType))
{
}

WGALSwapChain::~WGALSwapChain() = default;

//////////////////////////////////////////////////////////////////////////

WGALWindowSwapChain::Functor WGALWindowSwapChain::s_Factory;


WGALWindowSwapChain::WGALWindowSwapChain(const WGALWindowSwapChainCreationDescription& Description)
  : WGALSwapChain(WGetStaticRTTI<WGALWindowSwapChain>())
  , m_WindowDesc(Description)
{
}

void WGALWindowSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

WGALSwapChainHandle WGALWindowSwapChain::Create(const WGALWindowSwapChainCreationDescription& desc)
{
  W_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for WGALWindowSwapChain.");
  return s_Factory(desc);
}

W_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);

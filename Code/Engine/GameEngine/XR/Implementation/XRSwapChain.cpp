#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRSwapChain.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WGALXRSwapChain, WGALSwapChain, 1, WRTTINoAllocator)
{
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WGALXRSwapChain::Functor WGALXRSwapChain::s_Factory;

WGALXRSwapChain::WGALXRSwapChain(WXRInterface* pXrInterface)
  : WGALSwapChain(WGetStaticRTTI<WGALXRSwapChain>())
  , m_pXrInterface(pXrInterface)
{
}

WResult WGALXRSwapChain::UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode)
{
  return W_FAILURE;
}

void WGALXRSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

WGALSwapChainHandle WGALXRSwapChain::Create(WXRInterface* pXrInterface)
{
  W_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for WGALXRSwapChain.");
  return s_Factory(pXrInterface);
}


W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRSwapChain);

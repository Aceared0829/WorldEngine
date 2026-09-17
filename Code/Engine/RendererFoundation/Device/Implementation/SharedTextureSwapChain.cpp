#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WGALSharedTextureSwapChain, WGALSwapChain, 1, WRTTINoAllocator)
{
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WGALSharedTextureSwapChain::Functor WGALSharedTextureSwapChain::s_Factory;

void WGALSharedTextureSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

WGALSwapChainHandle WGALSharedTextureSwapChain::Create(const WGALSharedTextureSwapChainCreationDescription& desc)
{
  W_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for WGALWindowSwapChain.");
  return s_Factory(desc);
}

WGALSharedTextureSwapChain::WGALSharedTextureSwapChain(const WGALSharedTextureSwapChainCreationDescription& desc)
  : WGALSwapChain(WGetStaticRTTI<WGALSharedTextureSwapChain>())
  , m_Desc(desc)
{
}

void WGALSharedTextureSwapChain::Arm(WUInt32 uiTextureIndex, WUInt64 uiCurrentSemaphoreValue)
{
  if (m_uiCurrentTexture != WMath::MaxValue<WUInt32>())
  {
    // We did not use the previous texture index.
    m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
  }
  m_uiCurrentTexture = uiTextureIndex;
  m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
}

void WGALSharedTextureSwapChain::AcquireNextRenderTarget(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_ASSERT_DEV(m_uiCurrentTexture != WMath::MaxValue<WUInt32>(), "Acquire called without calling Arm first.");

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
  m_SharedTextureInterfaces[m_uiCurrentTexture]->WaitSemaphoreGPU(m_uiCurrentSemaphoreValue);
}

void WGALSharedTextureSwapChain::PresentRenderTarget(WGALDevice* pDevice)
{
  m_RenderTargets.m_hRTs[0].Invalidate();

  W_ASSERT_DEV(m_uiCurrentTexture != WMath::MaxValue<WUInt32>(), "Present called without calling Arm first.");

  m_SharedTextureInterfaces[m_uiCurrentTexture]->SignalSemaphoreGPU(m_uiCurrentSemaphoreValue + 1);
  m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 1);

  pDevice->Flush();

  m_uiCurrentTexture = WMath::MaxValue<WUInt32>();
}

WResult WGALSharedTextureSwapChain::UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode)
{
  W_IGNORE_UNUSED(pDevice);
  W_IGNORE_UNUSED(newPresentMode);

  return W_SUCCESS;
}

WResult WGALSharedTextureSwapChain::InitPlatform(WGALDevice* pDevice)
{
  // Create textures
  for (WUInt32 i = 0; i < m_Desc.m_Textures.GetCount(); ++i)
  {
    WGALPlatformSharedHandle handle = m_Desc.m_Textures[i];
    WGALTextureHandle hTexture = pDevice->OpenSharedTexture(m_Desc.m_TextureDesc, handle);
    if (hTexture.IsInvalidated())
    {
      WLog::Error("Failed to open shared texture");
      return W_FAILURE;
    }
    m_SharedTextureHandles.PushBack(hTexture);
    const WGALSharedTexture* pSharedTexture = pDevice->GetSharedTexture(hTexture);
    if (pSharedTexture == nullptr)
    {
      WLog::Error("Created texture is not a shared texture");
      return W_FAILURE;
    }
    m_SharedTextureInterfaces.PushBack(pSharedTexture);
    m_CurrentSemaphoreValue.PushBack(0);
  }
  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[0];
  m_CurrentSize = {m_Desc.m_TextureDesc.m_uiWidth, m_Desc.m_TextureDesc.m_uiHeight};
  return W_SUCCESS;
}

WResult WGALSharedTextureSwapChain::DeInitPlatform(WGALDevice* pDevice)
{
  for (WUInt32 i = 0; i < m_SharedTextureHandles.GetCount(); ++i)
  {
    pDevice->DestroySharedTexture(m_SharedTextureHandles[i]);
  }
  m_uiCurrentTexture = WMath::MaxValue<WUInt32>();
  m_uiCurrentSemaphoreValue = 0;
  m_SharedTextureHandles.Clear();
  m_SharedTextureInterfaces.Clear();
  m_CurrentSemaphoreValue.Clear();

  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SharedTextureSwapChain);

#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>

#include <d3d11.h>

//////////////////////////////////////////////////////////////////////////
// WGALSharedTextureDX11
//////////////////////////////////////////////////////////////////////////

WGALSharedTextureDX11::WGALSharedTextureDX11(const WGALTextureCreationDescription& Description, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle hSharedHandle)
  : WGALTextureDX11(Description)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

WGALSharedTextureDX11::~WGALSharedTextureDX11() = default;

WResult WGALSharedTextureDX11::InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_pDevice = pDXDevice;

  W_ASSERT_DEBUG(m_SharedType != WGALSharedTextureType::None, "Shared texture must either be exported or imported");
  W_ASSERT_DEBUG(m_Description.m_Type == WGALTextureType::Texture2DShared, "Shared texture must be of type WGALTextureType::Texture2DShared");

  if (m_SharedType == WGALSharedTextureType::Imported)
  {
    IDXGIResource* d3d11ResPtr = NULL;
    HRESULT hr = pDXDevice->GetDXDevice()->OpenSharedResource((HANDLE)m_hSharedHandle.m_hSharedTexture, __uuidof(ID3D11Resource), (void**)(&d3d11ResPtr));
    if (FAILED(hr))
    {
      WLog::Error("Failed to open shared texture: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }
    W_SCOPE_EXIT(d3d11ResPtr->Release());

    hr = d3d11ResPtr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pDXTexture));
    if (FAILED(hr))
    {
      WLog::Error("Failed to query shared texture interface: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }

    hr = d3d11ResPtr->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      WLog::Error("Failed to query keyed mutex interface: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  D3D11_TEXTURE2D_DESC Tex2DDesc = {};
  W_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

  if (m_SharedType == WGALSharedTextureType::Exported)
    Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

  WTempHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
  ConvertInitialData(m_Description, pInitialData, InitialData);

  if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
  {
    return W_FAILURE;
  }
  else if (m_SharedType == WGALSharedTextureType::Exported)
  {
    IDXGIResource* pDXGIResource;
    HRESULT hr = m_pDXTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
    if (FAILED(hr))
    {
      WLog::Error("Failed to get shared texture resource interface: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }
    W_SCOPE_EXIT(pDXGIResource->Release());
    HANDLE hTexture = 0;
    hr = pDXGIResource->GetSharedHandle(&hTexture);
    if (FAILED(hr))
    {
      WLog::Error("Failed to get shared handle: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }
    hr = pDXGIResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      WLog::Error("Failed to query keyed mutex interface: {}", WArgErrorCode(hr));
      return W_FAILURE;
    }
    m_hSharedHandle.m_hSharedTexture = (WUInt64)hTexture;
  }

  return W_SUCCESS;
}


WResult WGALSharedTextureDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_GAL_DX11_RELEASE(m_pKeyedMutex);
  return SUPER::DeInitPlatform(pDevice);
}

WGALPlatformSharedHandle WGALSharedTextureDX11::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void WGALSharedTextureDX11::WaitSemaphoreGPU(WUInt64 uiValue) const
{
  m_pKeyedMutex->AcquireSync(uiValue, INFINITE);
}

void WGALSharedTextureDX11::SignalSemaphoreGPU(WUInt64 uiValue) const
{
  m_pKeyedMutex->ReleaseSync(uiValue);
}

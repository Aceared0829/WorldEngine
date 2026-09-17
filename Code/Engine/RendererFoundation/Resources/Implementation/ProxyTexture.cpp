#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace
{
  WGALTextureCreationDescription MakeProxyDesc(const WGALTextureCreationDescription& parentDesc)
  {
    WGALTextureCreationDescription desc = parentDesc;
    desc.m_Type = WGALTextureType::Texture2DProxy;
    return desc;
  }
} // namespace

WGALProxyTexture::WGALProxyTexture(WGALTextureHandle hParentTexture, const WGALTexture& parentTexture, WUInt16 uiSlice)
  : WGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
  , m_hParentTexture(hParentTexture)
  , m_pParentTexture(&parentTexture)
  , m_uiSlice(uiSlice)
{
}

WGALProxyTexture::~WGALProxyTexture() = default;


const WGALResourceBase* WGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

WResult WGALProxyTexture::InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  W_IGNORE_UNUSED(pDevice);
  W_IGNORE_UNUSED(pInitialData);

  return W_SUCCESS;
}

WResult WGALProxyTexture::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  return W_SUCCESS;
}

void WGALProxyTexture::SetDebugNamePlatform(const char* szName) const
{
  W_IGNORE_UNUSED(szName);
}

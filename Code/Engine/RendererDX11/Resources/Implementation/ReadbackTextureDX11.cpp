#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

WGALReadbackTextureDX11::WGALReadbackTextureDX11(const WGALTextureCreationDescription& Description)
  : WGALReadbackTexture(Description)
{
}

WGALReadbackTextureDX11::~WGALReadbackTextureDX11() = default;

WResult WGALReadbackTextureDX11::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);

  switch (m_Description.m_Type)
  {
    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DArray:
    case WGALTextureType::TextureCube:
    case WGALTextureType::TextureCubeArray:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc = {};
      W_SUCCEED_OR_RETURN(WGALTextureDX11::Create2DDesc(m_Description, pDXDevice, Tex2DDesc));
      Tex2DDesc.BindFlags = 0;
      Tex2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Tex2DDesc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Tex2DDesc.Usage = D3D11_USAGE_STAGING;

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return W_FAILURE;
      }
    }
    break;

    case WGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc = {};
      W_SUCCEED_OR_RETURN(WGALTextureDX11::Create3DDesc(m_Description, pDXDevice, Tex3DDesc));
      Tex3DDesc.BindFlags = 0;
      Tex3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Tex3DDesc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Tex3DDesc.Usage = D3D11_USAGE_STAGING;

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(&Tex3DDesc, nullptr, reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
      {
        return W_FAILURE;
      }
    }
    break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WGALReadbackTextureDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXTexture);
  return W_SUCCESS;
}

void WGALReadbackTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  WUInt32 uiLength = WStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

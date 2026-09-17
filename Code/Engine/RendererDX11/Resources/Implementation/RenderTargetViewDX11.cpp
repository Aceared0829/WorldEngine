#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

WGALRenderTargetViewDX11::WGALRenderTargetViewDX11(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description)
  : WGALRenderTargetView(pTexture, Description)

{
}

WGALRenderTargetViewDX11::~WGALRenderTargetViewDX11() = default;

WResult WGALRenderTargetViewDX11::InitPlatform(WGALDevice* pDevice)
{
  const WGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    WLog::Error("No valid texture handle given for render target view creation!");
    return W_FAILURE;
  }

  const WGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  WGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != WGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;


  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;

  const bool bIsDepthFormat = WGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    WLog::Error("Couldn't get DXGI format for view!");
    return W_FAILURE;
  }

  ID3D11Resource* pDXResource = static_cast<const WGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    const WEnum<WGALTextureType> type = m_Description.m_OverrideViewType != WGALTextureType::Invalid ? m_Description.m_OverrideViewType : texDesc.m_Type;
    if (texDesc.m_SampleCount == WGALMSAASampleCount::None)
    {
      switch (type)
      {
        case WGALTextureType::Texture2D:
        case WGALTextureType::Texture2DShared:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
          break;

        case WGALTextureType::Texture2DProxy:
        case WGALTextureType::Texture2DArray:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
          DSViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
          DSViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          DSViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          W_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    else
    {
      switch (type)
      {
        case WGALTextureType::Texture2D:
        case WGALTextureType::Texture2DShared:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
          break;

        case WGALTextureType::Texture2DProxy:
        case WGALTextureType::Texture2DArray:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
          DSViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          DSViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          W_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pDXResource, &DSViewDesc, &m_pDepthStencilView)))
    {
      WLog::Error("Couldn't create depth stencil view!");
      return W_FAILURE;
    }
    else
    {
      return W_SUCCESS;
    }
  }
  else
  {
    D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
    RTViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == WGALMSAASampleCount::None)
    {
      switch (texDesc.m_Type)
      {
        case WGALTextureType::Texture2D:
        case WGALTextureType::Texture2DShared:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
          RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
          break;

        case WGALTextureType::Texture2DProxy:
        case WGALTextureType::Texture2DArray:
        case WGALTextureType::TextureCube:
        case WGALTextureType::TextureCubeArray:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
          RTViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
          RTViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          RTViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          W_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    else
    {
      switch (texDesc.m_Type)
      {
        case WGALTextureType::Texture2D:
        case WGALTextureType::Texture2DShared:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
          break;

        case WGALTextureType::Texture2DProxy:
        case WGALTextureType::Texture2DArray:
        case WGALTextureType::TextureCube:
        case WGALTextureType::TextureCubeArray:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
          RTViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          RTViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          W_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pDXResource, &RTViewDesc, &m_pRenderTargetView)))
    {
      WLog::Error("Couldn't create render target view!");
      return W_FAILURE;
    }
    else
    {
      return W_SUCCESS;
    }
  }
}

WResult WGALRenderTargetViewDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pRenderTargetView);
  W_GAL_DX11_RELEASE(m_pDepthStencilView);
  W_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return W_SUCCESS;
}

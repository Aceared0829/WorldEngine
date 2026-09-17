
#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

WGALTextureDX11::WGALTextureDX11(const WGALTextureCreationDescription& Description)
  : WGALTexture(Description)
{
}

WGALTextureDX11::~WGALTextureDX11() = default;

WResult WGALTextureDX11::InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> initialData)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_pDevice = pDXDevice;

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    return InitFromNativeObject(pDXDevice);
  }

  switch (m_Description.m_Type)
  {
    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DArray:
    case WGALTextureType::TextureCube:
    case WGALTextureType::TextureCubeArray:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc = {};
      W_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

      WTempHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, initialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, initialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return W_FAILURE;
      }
    }
    break;

    case WGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc = {};
      W_SUCCEED_OR_RETURN(Create3DDesc(m_Description, pDXDevice, Tex3DDesc));

      WTempHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, initialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(&Tex3DDesc, initialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
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


WResult WGALTextureDX11::InitFromNativeObject(WGALDeviceDX11* pDXDevice)
{
  W_IGNORE_UNUSED(pDXDevice);
  /// \todo Validation if interface of corresponding texture object exists
  m_pDXTexture = static_cast<ID3D11Resource*>(m_Description.m_pExisitingNativeObject);
  return W_SUCCESS;
}


WResult WGALTextureDX11::Create2DDesc(const WGALTextureCreationDescription& description, WGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_tex2DDesc)
{
  out_tex2DDesc.ArraySize = 1;
  out_tex2DDesc.MiscFlags = 0;
  out_tex2DDesc.BindFlags = 0;

  switch (description.m_Type)
  {
    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DShared:
      W_ASSERT_DEV(description.m_uiArraySize == 1, "Use WGALTextureType::Texture2DArray instead.");
      break;

    case WGALTextureType::Texture2DProxy:
    case WGALTextureType::Texture2DArray:
      out_tex2DDesc.ArraySize = description.m_uiArraySize;
      break;

    case WGALTextureType::Texture3D:
      W_ASSERT_DEV(description.m_uiArraySize == 1, "3D array textures not supported.");
      break;

    case WGALTextureType::TextureCube:
      W_ASSERT_DEV(description.m_uiArraySize == 1, "Use WGALTextureType::TextureCubeArray instead.");
      out_tex2DDesc.ArraySize = 6;
      out_tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
      break;

    case WGALTextureType::TextureCubeArray:
      out_tex2DDesc.ArraySize = description.m_uiArraySize * 6;
      out_tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource))
    out_tex2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess))
    out_tex2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
    out_tex2DDesc.BindFlags |= WGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_tex2DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_TextureFlags.IsAnySet(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::UnorderedAccess))
    out_tex2DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    WLog::Error("No storage format available for given format: {0}", description.m_Format);
    return W_FAILURE;
  }

  out_tex2DDesc.Width = description.m_uiWidth;
  out_tex2DDesc.Height = description.m_uiHeight;
  out_tex2DDesc.MipLevels = description.m_uiMipLevelCount;

  out_tex2DDesc.SampleDesc.Count = description.m_SampleCount;
  out_tex2DDesc.SampleDesc.Quality = 0;

  return W_SUCCESS;
}

WResult WGALTextureDX11::Create3DDesc(const WGALTextureCreationDescription& description, WGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_tex3DDesc)
{
  out_tex3DDesc.BindFlags = 0;

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource))
    out_tex3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess))
    out_tex3DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
    out_tex3DDesc.BindFlags |= WGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_tex3DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
  out_tex3DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_TextureFlags.IsAnySet(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::UnorderedAccess))
    out_tex3DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_tex3DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_tex3DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    WLog::Error("No storage format available for given format: {0}", description.m_Format);
    return W_FAILURE;
  }

  out_tex3DDesc.Width = description.m_uiWidth;
  out_tex3DDesc.Height = description.m_uiHeight;
  out_tex3DDesc.Depth = description.m_uiDepth;
  out_tex3DDesc.MipLevels = description.m_uiMipLevelCount;

  out_tex3DDesc.MiscFlags = 0;

  return W_SUCCESS;
}


void WGALTextureDX11::ConvertInitialData(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData, WHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_initialData)
{
  if (!initialData.IsEmpty())
  {
    WUInt32 uiArraySize = 1;
    switch (description.m_Type)
    {
      case WGALTextureType::Texture2D:
        W_ASSERT_DEV(description.m_uiArraySize == 1, "Use WGALTextureType::Texture2DArray instead.");
        break;
      case WGALTextureType::Texture2DArray:
        uiArraySize = description.m_uiArraySize;
        break;
      case WGALTextureType::TextureCube:
        W_ASSERT_DEV(description.m_uiArraySize == 1, "Use WGALTextureType::TextureCubeArray instead.");
        uiArraySize = 6;
        break;
      case WGALTextureType::TextureCubeArray:
        uiArraySize = description.m_uiArraySize * 6;
        break;

      default:
        W_ASSERT_DEV(description.m_uiArraySize == 1, "This type of texture doesn't support arrays.");
        break;
    }

    const WUInt32 uiInitialDataCount = (description.m_uiMipLevelCount * uiArraySize);

    W_ASSERT_DEV(initialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

    out_initialData.SetCountUninitialized(uiInitialDataCount);

    for (WUInt32 i = 0; i < uiInitialDataCount; i++)
    {
      out_initialData[i].pSysMem = initialData[i].m_pData.GetPtr();
      out_initialData[i].SysMemPitch = initialData[i].m_uiRowPitch;
      out_initialData[i].SysMemSlicePitch = initialData[i].m_uiSlicePitch;
    }
  }
}

ID3D11ShaderResourceView* WGALTextureDX11::GetSRV(WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType) const
{
  ID3D11ShaderResourceView* pSRV = nullptr;
  View view;
  view.m_TextureRange = textureRange;
  view.m_OverrideViewFormat = overrideViewFormat;
  view.m_OverrideViewType = overrideViewType;

  if (!m_SRVs.TryGetValue(view, pSRV))
  {
    const WGALResourceFormat::Enum viewFormat = overrideViewFormat != WGALResourceFormat::Invalid ? overrideViewFormat : m_Description.m_Format;
    const WEnum<WGALTextureType> type = overrideViewType != WGALTextureType::Invalid ? overrideViewType : m_Description.m_Type;

    if (textureRange.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES)
      textureRange.m_uiArraySlices = (WUInt16)m_Description.m_uiArraySize;

    DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
    if (WGALResourceFormat::IsDepthFormat(viewFormat))
    {
      DXViewFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthOnlyType;
    }
    else
    {
      DXViewFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    }

    if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
    {
      WLog::Error("Couldn't get valid DXGI format for resource view! ({0})", viewFormat);
      return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
    DXSRVDesc.Format = DXViewFormat;

    ID3D11Resource* pDXResource = GetDXTexture();
    const WGALTextureCreationDescription& texDesc = GetDescription();

    switch (type)
    {
      case WGALTextureType::Texture2D:
      case WGALTextureType::Texture2DShared:
        W_ASSERT_DEV(texDesc.m_uiArraySize == 1 && textureRange.m_uiBaseArraySlice == 0, "These options can only be used with array texture types.");

        if (texDesc.m_SampleCount == WGALMSAASampleCount::None)
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
          DXSRVDesc.Texture2D.MostDetailedMip = textureRange.m_uiBaseMipLevel;
          DXSRVDesc.Texture2D.MipLevels = textureRange.m_uiMipLevels;
        }
        else
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }
        break;

      case WGALTextureType::Texture2DProxy:
      case WGALTextureType::Texture2DArray:

        if (texDesc.m_SampleCount == WGALMSAASampleCount::None)
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
          DXSRVDesc.Texture2DArray.MostDetailedMip = textureRange.m_uiBaseMipLevel;
          DXSRVDesc.Texture2DArray.MipLevels = textureRange.m_uiMipLevels;
          DXSRVDesc.Texture2DArray.FirstArraySlice = textureRange.m_uiBaseArraySlice;
          DXSRVDesc.Texture2DArray.ArraySize = textureRange.m_uiArraySlices;
        }
        else
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
          DXSRVDesc.Texture2DMSArray.FirstArraySlice = textureRange.m_uiBaseArraySlice;
          DXSRVDesc.Texture2DMSArray.ArraySize = textureRange.m_uiArraySlices;
        }

        break;

      case WGALTextureType::TextureCube:
        W_ASSERT_DEV(texDesc.m_uiArraySize == 1 && textureRange.m_uiBaseArraySlice == 0, "These options can only be used with array texture types.");

        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        DXSRVDesc.TextureCube.MostDetailedMip = textureRange.m_uiBaseMipLevel;
        DXSRVDesc.TextureCube.MipLevels = textureRange.m_uiMipLevels;
        break;

      case WGALTextureType::TextureCubeArray:
        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        DXSRVDesc.TextureCubeArray.MostDetailedMip = textureRange.m_uiBaseMipLevel;
        DXSRVDesc.TextureCubeArray.MipLevels = textureRange.m_uiMipLevels;
        DXSRVDesc.TextureCubeArray.First2DArrayFace = textureRange.m_uiBaseArraySlice;
        DXSRVDesc.TextureCubeArray.NumCubes = textureRange.m_uiArraySlices / 6;
        break;

      case WGALTextureType::Texture3D:
        W_ASSERT_DEV(texDesc.m_uiArraySize == 1 && textureRange.m_uiBaseArraySlice == 0, "These options can only be used with array texture types.");

        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        DXSRVDesc.Texture3D.MostDetailedMip = textureRange.m_uiBaseMipLevel;
        DXSRVDesc.Texture3D.MipLevels = textureRange.m_uiMipLevels;
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        return nullptr;
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &pSRV)))
    {
      return nullptr;
    }

    m_SRVs.Insert(view, pSRV);
  }

  return pSRV;
}

ID3D11UnorderedAccessView* WGALTextureDX11::GetUAV(WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat) const
{
  ID3D11UnorderedAccessView* pUAV = nullptr;
  View view;
  view.m_TextureRange = textureRange;
  view.m_OverrideViewFormat = overrideViewFormat;

  if (!m_UAVs.TryGetValue(view, pUAV))
  {
    const WGALResourceFormat::Enum viewFormat = overrideViewFormat == WGALResourceFormat::Invalid ? m_Description.m_Format : overrideViewFormat;

    DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
    if (WGALResourceFormat::IsDepthFormat(viewFormat))
    {
      DXViewFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthOnlyType;
    }
    else
    {
      DXViewFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    }

    if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
    {
      WLog::Error("Couldn't get valid DXGI format for resource view! ({0})", viewFormat);
      return nullptr;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
    DXUAVDesc.Format = DXViewFormat;

    ID3D11Resource* pDXResource = GetDXTexture();
    const WGALTextureCreationDescription& texDesc = GetDescription();
    W_IGNORE_UNUSED(texDesc);

    // DX11 does not care about view types matching the shader. It does care though about view types matching the resource.
    const WEnum<WGALTextureType> type = texDesc.m_Type;
    switch (type)
    {
      case WGALTextureType::Texture2D:
      case WGALTextureType::Texture2DShared:
        W_ASSERT_DEV(texDesc.m_uiArraySize == 1 && textureRange.m_uiBaseArraySlice == 0, "These options can only be used with array texture types.");

        DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        DXUAVDesc.Texture2D.MipSlice = textureRange.m_uiBaseMipLevel;
        break;

      case WGALTextureType::Texture2DProxy:
      case WGALTextureType::Texture2DArray:
      case WGALTextureType::TextureCube:
      case WGALTextureType::TextureCubeArray:
        DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        DXUAVDesc.Texture2DArray.MipSlice = textureRange.m_uiBaseMipLevel;
        DXUAVDesc.Texture2DArray.FirstArraySlice = textureRange.m_uiBaseArraySlice;
        DXUAVDesc.Texture2DArray.ArraySize = textureRange.m_uiArraySlices;
        break;

      case WGALTextureType::Texture3D:
        DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        DXUAVDesc.Texture3D.MipSlice = textureRange.m_uiBaseMipLevel;
        DXUAVDesc.Texture3D.FirstWSlice = textureRange.m_uiBaseArraySlice;
        DXUAVDesc.Texture3D.WSize = textureRange.m_uiArraySlices;
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        return nullptr;
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &pUAV)))
    {
      return nullptr;
    }

    m_UAVs.Insert(view, pUAV);
  }
  return pUAV;
}

WResult WGALTextureDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXTexture);

  for (auto it : m_SRVs)
  {
    W_GAL_DX11_RELEASE(it.Value());
  }
  m_SRVs.Clear();
  for (auto it : m_UAVs)
  {
    W_GAL_DX11_RELEASE(it.Value());
  }
  m_UAVs.Clear();
  return W_SUCCESS;
}

void WGALTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  WUInt32 uiLength = WStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

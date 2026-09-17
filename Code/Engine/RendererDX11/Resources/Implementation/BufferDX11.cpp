#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/Resources/BufferDX11.h>

#include <d3d11.h>


WGALBufferDX11::WGALBufferDX11(const WGALBufferCreationDescription& Description)
  : WGALBuffer(Description)
{
}

WGALBufferDX11::~WGALBufferDX11() = default;

WResult WGALBufferDX11::CreateBufferDesc(const WGALBufferCreationDescription& description, D3D11_BUFFER_DESC& out_bufferDesc, DXGI_FORMAT& out_indexFormat)
{
  for (WGALBufferUsageFlags::Enum flag : description.m_BufferFlags)
  {
    switch (flag)
    {
      case WGALBufferUsageFlags::ConstantBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
        break;
      case WGALBufferUsageFlags::IndexBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
        out_indexFormat = description.m_uiStructSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        break;
      case WGALBufferUsageFlags::VertexBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
        break;
      case WGALBufferUsageFlags::TexelBuffer:
        break;
      case WGALBufferUsageFlags::StructuredBuffer:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        break;
      case WGALBufferUsageFlags::ByteAddressBuffer:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        break;
      case WGALBufferUsageFlags::ShaderResource:
        out_bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        break;
      case WGALBufferUsageFlags::UnorderedAccess:
        out_bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        break;
      case WGALBufferUsageFlags::DrawIndirect:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        break;
      case WGALBufferUsageFlags::Transient:
        // Nothing to set here. We only use this flag to decide whether its safe to use D3D11_MAP_WRITE_NO_OVERWRITE / D3D11_MAP_WRITE_DISCARD inside WGALCommandEncoderImplDX11::UpdateBufferPlatform.
        break;
      default:
        WLog::Error("Unknown buffer type supplied to CreateBuffer()!");
        return W_FAILURE;
    }
  }

  out_bufferDesc.ByteWidth = description.m_uiTotalSize;
  out_bufferDesc.CPUAccessFlags = 0;
  out_bufferDesc.StructureByteStride = description.m_uiStructSize;

  out_bufferDesc.CPUAccessFlags = 0;
  if (description.m_BufferFlags.IsSet(WGALBufferUsageFlags::ConstantBuffer))
  {
    out_bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    out_bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    out_bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

    // If constant buffer: Patch size to be aligned to 64 bytes for easier usability
    out_bufferDesc.ByteWidth = WMemoryUtils::AlignSize(out_bufferDesc.ByteWidth, 64u);
  }
  else
  {
    if (description.m_ResourceAccess.IsImmutable())
    {
      out_bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else
    {
      if (description.m_BufferFlags.IsSet(WGALBufferUsageFlags::UnorderedAccess)) // UAVs allow writing from the GPU which cannot be combined with CPU write access.
      {
        out_bufferDesc.Usage = D3D11_USAGE_DEFAULT;
      }
      else
      {
        out_bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        out_bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      }
    }
  }
  return W_SUCCESS;
}

ID3D11ShaderResourceView* WGALBufferDX11::GetSRV(WGALBufferRange bufferRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const
{
  ID3D11ShaderResourceView* pSRV = nullptr;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_ResourceType = resourceType;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_SRVs.TryGetValue(view, pSRV))
  {
    ID3D11Resource* pDXResource = GetDXBuffer();
    const WGALBufferCreationDescription& bufferDesc = GetDescription();

    D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
    DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    DXSRVDesc.BufferEx.Flags = 0;

    switch (resourceType)
    {
      case WGALShaderResourceType::TexelBuffer:
      {
        const WGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == WGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

        const auto& formatInfo = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
        const WUInt32 uiBytesPerElement = WGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / uiBytesPerElement;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / uiBytesPerElement;
        DXSRVDesc.Format = WGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
        if (DXSRVDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
          WLog::Error("Couldn't get valid DXGI format for resource view! ({0})", viewFormat);
          return nullptr;
        }
      }
      break;
      case WGALShaderResourceType::StructuredBuffer:
      {
        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / bufferDesc.m_uiStructSize;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / bufferDesc.m_uiStructSize;
      }
      break;
      case WGALShaderResourceType::ByteAddressBuffer:
      {
        DXSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / 4;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / 4;
        DXSRVDesc.BufferEx.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
      }
      break;
      default:
        W_REPORT_FAILURE("Unsupported resource type: {}", (WUInt32)resourceType);
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &pSRV)))
    {
      return nullptr;
    }
    m_SRVs.Insert(view, pSRV);
  }

  return pSRV;
}

ID3D11UnorderedAccessView* WGALBufferDX11::GetUAV(WGALBufferRange bufferRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const
{
  ID3D11UnorderedAccessView* pUAV = nullptr;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_UAVs.TryGetValue(view, pUAV))
  {
    ID3D11Resource* pDXResource = GetDXBuffer();
    const WGALBufferCreationDescription& bufferDesc = GetDescription();

    D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
    DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DXUAVDesc.Buffer.Flags = 0;
    switch (resourceType)
    {
      case WGALShaderResourceType::TexelBufferRW:
      {
        const WGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == WGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

        const auto& formatInfo = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
        const WUInt32 uiBytesPerElement = WGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / uiBytesPerElement;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / uiBytesPerElement;
        DXUAVDesc.Format = WGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
        if (DXUAVDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
          WLog::Error("Couldn't get valid DXGI format for unordered access view! ({0})", viewFormat);
          return nullptr;
        }
      }
      break;
      case WGALShaderResourceType::StructuredBufferRW:
      {
        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / bufferDesc.m_uiStructSize;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / bufferDesc.m_uiStructSize;
      }
      break;
      case WGALShaderResourceType::ByteAddressBufferRW:
      {
        DXUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / 4;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / 4;
        DXUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
      }
      break;
      default:
        W_REPORT_FAILURE("Unsupported resource type: {}", (WUInt32)resourceType);
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &pUAV)))
    {
      return nullptr;
    }

    m_UAVs.Insert(view, pUAV);
  }

  return pUAV;
}

WResult WGALBufferDX11::InitPlatform(WGALDevice* pDevice, WArrayPtr<const WUInt8> pInitialData)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_pDevice = pDXDevice;

  D3D11_BUFFER_DESC BufferDesc = {};
  W_SUCCEED_OR_RETURN(CreateBufferDesc(m_Description, BufferDesc, m_IndexFormat));

  D3D11_SUBRESOURCE_DATA DXInitialData;
  DXInitialData.pSysMem = pInitialData.GetPtr();
  DXInitialData.SysMemPitch = DXInitialData.SysMemSlicePitch = 0;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateBuffer(&BufferDesc, pInitialData.IsEmpty() ? nullptr : &DXInitialData, &m_pDXBuffer)))
  {
    return W_SUCCESS;
  }
  else
  {
    WLog::Error("Creation of native DirectX buffer failed!");
    return W_FAILURE;
  }
}

WResult WGALBufferDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);
  W_GAL_DX11_RELEASE(m_pDXBuffer);

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

void WGALBufferDX11::SetDebugNamePlatform(const char* szName) const
{
  WUInt32 uiLength = WStringUtils::GetStringElementCount(szName);

  if (m_pDXBuffer != nullptr)
  {
    m_pDXBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

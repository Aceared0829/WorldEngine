#include <RendererDX11/RendererDX11PCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <RendererDX11/Pools/QueryPoolDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Shader/BindGroupLayoutDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/ComputePipelineDX11.h>
#include <RendererDX11/State/GraphicsPipelineDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

WGALCommandEncoderImplDX11::WGALCommandEncoderImplDX11(WGALDeviceDX11& ref_deviceDX11)
  : m_GALDeviceDX11(ref_deviceDX11)
{
  m_pDXContext = m_GALDeviceDX11.GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    WLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

WGALCommandEncoderImplDX11::~WGALCommandEncoderImplDX11()
{
  W_GAL_DX11_RELEASE(m_pDXAnnotation);
}


void WGALCommandEncoderImplDX11::EndFrame()
{
  m_AlreadyUpdatedTransientBuffers.Clear();
}

// State setting functions

void WGALCommandEncoderImplDX11::SetShader(const WGALShader* pShader)
{
  m_uiTessellationPatchControlPoints = 0;
  ID3D11VertexShader* pVS = nullptr;
  ID3D11HullShader* pHS = nullptr;
  ID3D11DomainShader* pDS = nullptr;
  ID3D11GeometryShader* pGS = nullptr;
  ID3D11PixelShader* pPS = nullptr;
  ID3D11ComputeShader* pCS = nullptr;

  if (pShader != nullptr)
  {
    const WGALShaderDX11* pDXShader = static_cast<const WGALShaderDX11*>(pShader);

    pVS = pDXShader->GetDXVertexShader();
    pHS = pDXShader->GetDXHullShader();
    pDS = pDXShader->GetDXDomainShader();
    pGS = pDXShader->GetDXGeometryShader();
    pPS = pDXShader->GetDXPixelShader();
    pCS = pDXShader->GetDXComputeShader();
  }

  if (pVS != m_pBoundShaders[WGALShaderStage::VertexShader])
  {
    m_pDXContext->VSSetShader(pVS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::VertexShader] = pVS;
  }

  if (pHS != m_pBoundShaders[WGALShaderStage::HullShader])
  {
    m_pDXContext->HSSetShader(pHS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::HullShader] = pHS;
    if (pHS)
    {
      m_uiTessellationPatchControlPoints = pShader->GetDescription().m_ByteCodes[WGALShaderStage::HullShader]->m_uiTessellationPatchControlPoints;
    }
    else
    {
      m_uiTessellationPatchControlPoints = 0;
    }
  }

  if (pDS != m_pBoundShaders[WGALShaderStage::DomainShader])
  {
    m_pDXContext->DSSetShader(pDS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::DomainShader] = pDS;
  }

  if (pGS != m_pBoundShaders[WGALShaderStage::GeometryShader])
  {
    m_pDXContext->GSSetShader(pGS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::GeometryShader] = pGS;
  }

  if (pPS != m_pBoundShaders[WGALShaderStage::PixelShader])
  {
    m_pDXContext->PSSetShader(pPS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::PixelShader] = pPS;
  }

  if (pCS != m_pBoundShaders[WGALShaderStage::ComputeShader])
  {
    m_pDXContext->CSSetShader(pCS, nullptr, 0);
    m_pBoundShaders[WGALShaderStage::ComputeShader] = pCS;
  }
}

void WGALCommandEncoderImplDX11::SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroupCreationDescription& bindGroup)
{
  W_IGNORE_UNUSED(uiBindGroup);
  const WGALBindGroupLayoutDX11* pLayout = static_cast<const WGALBindGroupLayoutDX11*>(m_GALDeviceDX11.GetBindGroupLayout(bindGroup.m_hBindGroupLayout));
  WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const WUInt32 uiBindings = bindings.GetCount();
  for (WUInt32 i = 0; i < uiBindings; ++i)
  {
    const WShaderResourceBinding& binding = bindings[i];
    const WGALBindGroupItem& item = bindGroup.m_BindGroupItems[i];

    switch (binding.m_ResourceType)
    {
      case WGALShaderResourceType::ConstantBuffer:
      {
        const WGALBufferDX11* pBuffer = static_cast<const WGALBufferDX11*>(m_GALDeviceDX11.GetBuffer(item.m_Buffer.m_hBuffer));
        if (item.m_Flags.IsSet(WGALBindGroupItemFlags::EmptyBinding))
          pBuffer = nullptr;

        SetConstantBuffer(binding, pBuffer);
      }
      break;
      case WGALShaderResourceType::Texture:
      {
        const WGALTextureDX11* pTexture = static_cast<const WGALTextureDX11*>(m_GALDeviceDX11.GetTexture(item.m_Texture.m_hTexture));
        if (item.m_Flags.IsSet(WGALBindGroupItemFlags::EmptyBinding))
          pTexture = nullptr;

        if (pTexture != nullptr && UnsetUnorderedAccessViews(pTexture))
        {
          FlushDeferredStateChanges().IgnoreResult();
        }

        ID3D11ShaderResourceView* pResourceViewDX11 = pTexture != nullptr ? pTexture->GetSRV(item.m_Texture.m_TextureRange, item.m_Texture.m_OverrideViewFormat, item.m_Texture.m_OverrideViewType) : nullptr;

        SetResourceView(binding, pTexture, pResourceViewDX11);
      }
      break;
      case WGALShaderResourceType::TextureRW:
      {
        const WGALTextureDX11* pTexture = static_cast<const WGALTextureDX11*>(m_GALDeviceDX11.GetTexture(item.m_Texture.m_hTexture));
        if (item.m_Flags.IsSet(WGALBindGroupItemFlags::EmptyBinding))
          pTexture = nullptr;

        if (pTexture != nullptr && UnsetResourceViews(pTexture))
        {
          FlushDeferredStateChanges().IgnoreResult();
        }

        ID3D11UnorderedAccessView* pUnorderedAccessViewDX11 = pTexture ? pTexture->GetUAV(item.m_Texture.m_TextureRange, item.m_Texture.m_OverrideViewFormat) : nullptr;
        SetUnorderedAccessView(binding, pUnorderedAccessViewDX11, pTexture);
      }
      break;

      case WGALShaderResourceType::TexelBuffer:
      case WGALShaderResourceType::StructuredBuffer:
      case WGALShaderResourceType::ByteAddressBuffer:
      {
        const WGALBufferDX11* pBuffer = static_cast<const WGALBufferDX11*>(m_GALDeviceDX11.GetBuffer(item.m_Buffer.m_hBuffer));
        if (item.m_Flags.IsSet(WGALBindGroupItemFlags::EmptyBinding))
          pBuffer = nullptr;

        if (pBuffer != nullptr && UnsetUnorderedAccessViews(pBuffer))
        {
          FlushDeferredStateChanges().IgnoreResult();
        }

        ID3D11ShaderResourceView* pResourceViewDX11 = pBuffer != nullptr ? pBuffer->GetSRV(item.m_Buffer.m_BufferRange, binding.m_ResourceType, item.m_Buffer.m_OverrideTexelBufferFormat) : nullptr;

        SetResourceView(binding, pBuffer, pResourceViewDX11);
      }
      break;

      case WGALShaderResourceType::TexelBufferRW:
      case WGALShaderResourceType::StructuredBufferRW:
      case WGALShaderResourceType::ByteAddressBufferRW:
      {
        const WGALBufferDX11* pBuffer = static_cast<const WGALBufferDX11*>(m_GALDeviceDX11.GetBuffer(item.m_Buffer.m_hBuffer));
        if (item.m_Flags.IsSet(WGALBindGroupItemFlags::EmptyBinding))
          pBuffer = nullptr;

        if (pBuffer != nullptr && UnsetResourceViews(pBuffer))
        {
          FlushDeferredStateChanges().IgnoreResult();
        }

        ID3D11UnorderedAccessView* pUnorderedAccessViewDX11 = pBuffer != nullptr ? pBuffer->GetUAV(item.m_Buffer.m_BufferRange, binding.m_ResourceType, item.m_Buffer.m_OverrideTexelBufferFormat) : nullptr;
        SetUnorderedAccessView(binding, pUnorderedAccessViewDX11, pBuffer);
      }
      break;
      case WGALShaderResourceType::Sampler:
      {
        const WGALSamplerStateDX11* pSampler = static_cast<const WGALSamplerStateDX11*>(m_GALDeviceDX11.GetSamplerState(item.m_Sampler.m_hSampler));
        SetSamplerState(binding, pSampler);
      }
      break;
      case WGALShaderResourceType::TextureAndSampler:
      default:
        break;
    }
  }
}

void WGALCommandEncoderImplDX11::SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroup* pBindGroup)
{
  // There is no way to persist bind groups in DX11, so this just redirects to the transient bind group code path.
  SetBindGroupPlatform(uiBindGroup, pBindGroup->GetDescription());
}

void WGALCommandEncoderImplDX11::SetConstantBuffer(const WShaderResourceBinding& binding, const WGALBuffer* pBuffer)
{
  W_ASSERT_RELEASE(binding.m_iSlot < W_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index too big!");

  ID3D11Buffer* pBufferDX11 = pBuffer != nullptr ? static_cast<const WGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;
  if (m_pBoundConstantBuffers[binding.m_iSlot] == pBufferDX11)
    return;

  m_pBoundConstantBuffers[binding.m_iSlot] = pBufferDX11;
  // The GAL doesn't care about stages for constant buffer, but we need to handle this internally.
  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(binding.m_iSlot);
}

void WGALCommandEncoderImplDX11::SetSamplerState(const WShaderResourceBinding& binding, const WGALSamplerState* pSamplerState)
{
  W_ASSERT_RELEASE(binding.m_iSlot < W_GAL_MAX_SAMPLER_COUNT, "Sampler state slot index too big!");

  ID3D11SamplerState* pSamplerStateDX11 = pSamplerState != nullptr ? static_cast<const WGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;

  for (WGALShaderStage::Enum stage : WIterateBitIndices<WUInt16, WGALShaderStage::Enum>(binding.m_Stages.GetValue()))
  {
    if (m_pBoundSamplerStates[stage][binding.m_iSlot] != pSamplerStateDX11)
    {
      m_pBoundSamplerStates[stage][binding.m_iSlot] = pSamplerStateDX11;
      m_BoundSamplerStatesRange[stage].SetToIncludeValue(binding.m_iSlot);
    }
  }
}


void WGALCommandEncoderImplDX11::SetResourceView(const WShaderResourceBinding& binding, const WGALResourceBase* pResource, ID3D11ShaderResourceView* pResourceViewDX11)
{
  for (WGALShaderStage::Enum stage : WIterateBitIndices<WUInt16, WGALShaderStage::Enum>(binding.m_Stages.GetValue()))
  {
    auto& boundShaderResourceViews = m_pBoundShaderResourceViews[stage];
    boundShaderResourceViews.EnsureCount(binding.m_iSlot + 1);
    auto& resourcesForResourceViews = m_ResourcesForResourceViews[stage];
    resourcesForResourceViews.EnsureCount(binding.m_iSlot + 1);
    if (boundShaderResourceViews[binding.m_iSlot] != pResourceViewDX11)
    {
      boundShaderResourceViews[binding.m_iSlot] = pResourceViewDX11;
      resourcesForResourceViews[binding.m_iSlot] = pResource;
      m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(binding.m_iSlot);
    }
  }
}

void WGALCommandEncoderImplDX11::SetUnorderedAccessView(const WShaderResourceBinding& binding, ID3D11UnorderedAccessView* pUnorderedAccessViewDX11, const WGALResourceBase* pResource)
{
  m_BoundUnorderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  m_ResourcesForUnorderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  if (m_BoundUnorderedAccessViews[binding.m_iSlot] != pUnorderedAccessViewDX11)
  {
    m_BoundUnorderedAccessViews[binding.m_iSlot] = pUnorderedAccessViewDX11;
    m_ResourcesForUnorderedAccessViews[binding.m_iSlot] = pResource;
    m_BoundUnorderedAccessViewsRange.SetToIncludeValue(binding.m_iSlot);
  }
}

void WGALCommandEncoderImplDX11::SetPushConstantsPlatform(WArrayPtr<const WUInt8> data)
{
  W_IGNORE_UNUSED(data);
  W_REPORT_FAILURE("DX11 does not support push constants, this function should not have been called.");
}

// Query functions

WGALTimestampHandle WGALCommandEncoderImplDX11::InsertTimestampPlatform()
{
  return m_GALDeviceDX11.GetQueryPool().InsertTimestamp();
}

WGALOcclusionHandle WGALCommandEncoderImplDX11::BeginOcclusionQueryPlatform(WEnum<WGALQueryType> type)
{
  return m_GALDeviceDX11.GetQueryPool().BeginOcclusionQuery(type);
}

void WGALCommandEncoderImplDX11::EndOcclusionQueryPlatform(WGALOcclusionHandle hOcclusion)
{
  m_GALDeviceDX11.GetQueryPool().EndOcclusionQuery(hOcclusion);
}

WGALFenceHandle WGALCommandEncoderImplDX11::InsertFencePlatform()
{
  return m_GALDeviceDX11.GetFenceQueue().GetCurrentFenceHandle();
}

// Resource update functions

void WGALCommandEncoderImplDX11::CopyBufferPlatform(const WGALBuffer* pDestination, const WGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const WGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const WGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void WGALCommandEncoderImplDX11::CopyBufferRegionPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, const WGALBuffer* pSource, WUInt32 uiSourceOffset, WUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const WGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const WGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void WGALCommandEncoderImplDX11::UpdateBufferPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, WArrayPtr<const WUInt8> sourceData, WGALUpdateMode::Enum updateMode)
{
  ID3D11Buffer* pDXDestination = static_cast<const WGALBufferDX11*>(pDestination)->GetDXBuffer();

  // On DX11 we can treat non-transient and transient constant buffers equally.
  if (updateMode == WGALUpdateMode::TransientConstantBuffer || pDestination->GetDescription().m_BufferFlags.IsSet(WGALBufferUsageFlags::ConstantBuffer))
  {
    W_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    const bool bTransient = pDestination->GetDescription().m_BufferFlags.IsSet(WGALBufferUsageFlags::Transient);
    if (!bTransient)
    {
      if (WGALDeviceDX11::TempResource tempResource = m_GALDeviceDX11.CopyToTempBuffer(sourceData))
      {
        m_GALDeviceDX11.UnmapTempResource(tempResource);

        D3D11_BOX srcBox = {0, 0, 0, sourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, tempResource.m_pResource, 0, &srcBox);
      }
      else
      {
        W_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
      if (!m_AlreadyUpdatedTransientBuffers.Contains(pDestination))
      {
        // If this is the first time we update a transient buffer this frame, we can use DISCARD which will allow us to safely use NO_OVERWRITE on this buffer for this frame afterwards.
        // This is guaranteed by the constraint that the buffer must not be updated twice on the same memory location within one frame.
        m_AlreadyUpdatedTransientBuffers.Insert(pDestination);
        mapType = D3D11_MAP_WRITE_DISCARD;
      }

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(WMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
      else
      {
        WLog::Error("Could not map buffer to update content.");
      }
    }
  }
}

void WGALCommandEncoderImplDX11::CopyTexturePlatform(const WGALTexture* pDestination, const WGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const WGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const WGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void WGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource,
  const WVec3U32& vDestinationPoint, const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource, const WBoundingBoxu32& box)
{
  ID3D11Resource* pDXDestination = static_cast<const WGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const WGALTextureDX11*>(pSource)->GetDXTexture();

  WUInt32 dstSubResource = D3D11CalcSubresource(
    destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  WUInt32 srcSubResource =
    D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {box.m_vMin.x, box.m_vMin.y, box.m_vMin.z, box.m_vMax.x, box.m_vMax.y, box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void WGALCommandEncoderImplDX11::UpdateTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource,
  const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const WGALTextureDX11*>(pDestination)->GetDXTexture();

  WUInt32 uiWidth = WMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u);
  WUInt32 uiHeight = WMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u);
  WUInt32 uiDepth = WMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u);
  WGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (WGALDeviceDX11::TempResource tempResource = m_GALDeviceDX11.CopyToTempTexture(sourceData, uiWidth, uiHeight, uiDepth, format))
  {
    m_GALDeviceDX11.UnmapTempResource(tempResource);

    WUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, destinationBox.m_vMin.x, destinationBox.m_vMin.y, destinationBox.m_vMin.z, tempResource.m_pResource, 0, &srcBox);
  }
  else
  {
    W_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void WGALCommandEncoderImplDX11::ResolveTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource,
  const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const WGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const WGALTextureDX11*>(pSource)->GetDXTexture();

  WUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  WUInt32 srcSubResource = D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void WGALCommandEncoderImplDX11::ReadbackTexturePlatform(const WGALReadbackTexture* pDestination, const WGALTexture* pSource)
{
  const WGALReadbackTextureDX11* pDXDestination = static_cast<const WGALReadbackTextureDX11*>(pDestination);
  const WGALTextureDX11* pDXTexture = static_cast<const WGALTextureDX11*>(pSource);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != WGALMSAASampleCount::None;
  W_IGNORE_UNUSED(bMSAASourceTexture);
  W_ASSERT_DEV(!bMSAASourceTexture, "MSAA readback is not supported");
  m_pDXContext->CopyResource(pDXDestination->GetDXTexture(), pDXTexture->GetDXTexture());
}


void WGALCommandEncoderImplDX11::ReadbackBufferPlatform(const WGALReadbackBuffer* pDestination, const WGALBuffer* pSource)
{
  const WGALReadbackBufferDX11* pDXDestination = static_cast<const WGALReadbackBufferDX11*>(pDestination);
  const WGALBufferDX11* pDXBuffer = static_cast<const WGALBufferDX11*>(pSource);
  m_pDXContext->CopyResource(pDXDestination->GetDXBuffer(), pDXBuffer->GetDXBuffer());
}

void WGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges().IgnoreResult();
  m_GALDeviceDX11.GetFenceQueue().SubmitCurrentFence();
  m_pDXContext->Flush();
}

void WGALCommandEncoderImplDX11::TextureBarrierPlatform(WArrayPtr<const WGALTextureBarrier> /*barriers*/)
{
  // DX11 does not support explicit barriers.
}

void WGALCommandEncoderImplDX11::BufferBarrierPlatform(WArrayPtr<const WGALBufferBarrier> /*barriers*/)
{
  // DX11 does not support explicit barriers.
}

// Debug helper functions

void WGALCommandEncoderImplDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    WStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void WGALCommandEncoderImplDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void WGALCommandEncoderImplDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    WStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

void WGALCommandEncoderImplDX11::BeginRenderingPlatform(const WGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup)
  {
    m_RenderTargetSetup = renderingSetup;

    const WGALRenderTargetView* pRenderTargetViews[W_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const WGALRenderTargetView* pDepthStencilView = nullptr;

    const WUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetColorTargetCount();

    bool bFlushNeeded = false;

    for (WUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const WGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetFrameBuffer().m_hColorTarget[uiIndex]);
      if (pRenderTargetView != nullptr)
      {
        const WGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= UnsetResourceViews(pTexture);
        bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    if (m_RenderTargetSetup.HasDepthStencilTarget())
    {
      pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetFrameBuffer().m_hDepthTarget);
      if (pDepthStencilView != nullptr)
      {
        const WGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

        bFlushNeeded |= UnsetResourceViews(pTexture);
        bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
      }
    }

    if (bFlushNeeded)
    {
      FlushDeferredStateChanges().IgnoreResult();
    }

    for (WUInt32 i = 0; i < W_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (WUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<const WGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const WGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(WMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  for (WUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (m_RenderTargetSetup.GetRenderPass().m_ColorLoadOp[i] == WGALRenderTargetLoadOp::Clear && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], m_RenderTargetSetup.GetClearColor((WUInt8)i).GetData());
    }
  }

  bool bClearDepth = m_RenderTargetSetup.GetRenderPass().m_DepthLoadOp == WGALRenderTargetLoadOp::Clear;
  bool bClearStencil = m_RenderTargetSetup.GetRenderPass().m_StencilLoadOp == WGALRenderTargetLoadOp::Clear;
  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    WUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, m_RenderTargetSetup.GetClearDepth(), m_RenderTargetSetup.GetClearStencil());
  }
}

void WGALCommandEncoderImplDX11::EndRenderingPlatform()
{
}

void WGALCommandEncoderImplDX11::BeginComputePlatform()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = WGALRenderingSetup();
  m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
}
void WGALCommandEncoderImplDX11::EndComputePlatform()
{
}

// Draw functions

void WGALCommandEncoderImplDX11::ClearPlatform(const WColor& clearColor, WUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, WUInt8 uiStencilClear)
{
  for (WUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], clearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    WUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

WResult WGALCommandEncoderImplDX11::DrawPlatform(WUInt32 uiVertexCount, WUInt32 uiStartVertex)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DrawIndexedPlatform(WUInt32 uiIndexCount, WUInt32 uiStartIndex)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    if (m_GALDeviceDX11.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_GALDeviceDX11.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(WUInt32 uiIndexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartIndex)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const WGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DrawInstancedPlatform(WUInt32 uiVertexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartVertex)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawInstancedIndirect(static_cast<const WGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return W_SUCCESS;
}

void WGALCommandEncoderImplDX11::SetIndexBufferPlatform(const WGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const WGALBufferDX11* pDX11Buffer = static_cast<const WGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void WGALCommandEncoderImplDX11::SetVertexBufferPlatform(WUInt32 uiSlot, const WGALBuffer* pVertexBuffer, WUInt32 uiOffset)
{
  W_ASSERT_DEV(uiSlot < W_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const WGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferOffsets[uiSlot] = uiOffset;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void WGALCommandEncoderImplDX11::SetGraphicsPipelinePlatform(const WGALGraphicsPipeline* pGraphicsPipeline)
{
  const WGALShader* pShader = nullptr;
  const WGALVertexDeclaration* pVertexDeclaration = nullptr;
  const WGALRasterizerState* pRasterizerState = nullptr;
  const WGALBlendState* pBlendState = nullptr;
  const WGALDepthStencilState* pDepthStencilState = nullptr;

  if (pGraphicsPipeline)
  {
    const WGALGraphicsPipelineCreationDescription& desc = pGraphicsPipeline->GetDescription();
    pShader = m_GALDeviceDX11.GetShader(desc.m_hShader);
    W_ASSERT_DEBUG(pShader->GetDescription().m_ByteCodes[WGALShaderStage::ComputeShader] == nullptr, "");
    pVertexDeclaration = m_GALDeviceDX11.GetVertexDeclaration(desc.m_hVertexDeclaration);
    pRasterizerState = m_GALDeviceDX11.GetRasterizerState(desc.m_hRasterizerState);
    pBlendState = m_GALDeviceDX11.GetBlendState(desc.m_hBlendState);
    pDepthStencilState = m_GALDeviceDX11.GetDepthStencilState(desc.m_hDepthStencilState);
    SetPrimitiveTopology(desc.m_Topology);
  }

  SetShader(pShader);
  SetVertexDeclaration(pVertexDeclaration);
  SetRasterizerState(pRasterizerState);
  SetBlendState(pBlendState);
  SetDepthStencilState(pDepthStencilState);
}

void WGALCommandEncoderImplDX11::SetComputePipelinePlatform(const WGALComputePipeline* pComputePipeline)
{
  const WGALShader* pShader = nullptr;

  if (pComputePipeline)
  {
    const WGALComputePipelineCreationDescription& desc = pComputePipeline->GetDescription();
    pShader = m_GALDeviceDX11.GetShader(desc.m_hShader);
    W_ASSERT_DEBUG(pShader->GetDescription().m_ByteCodes[WGALShaderStage::ComputeShader] != nullptr, "");
  }

  SetShader(pShader);
}

void WGALCommandEncoderImplDX11::SetVertexDeclaration(const WGALVertexDeclaration* pVertexDeclaration)
{
  WMemoryUtils::ZeroFill(m_VertexBufferStrides, W_ARRAY_SIZE(m_VertexBufferStrides));
  auto pVertexDeclarationDX11 = static_cast<const WGALVertexDeclarationDX11*>(pVertexDeclaration);
  if (pVertexDeclaration)
  {
    WArrayPtr<const WUInt32> strides = pVertexDeclarationDX11->GetVertexBufferStrides();
    if (!strides.IsEmpty())
    {
      m_BoundVertexBuffersRange.SetToIncludeValue(0);
      m_BoundVertexBuffersRange.SetToIncludeValue(strides.GetCount() - 1);
    }
    WMemoryUtils::Copy(m_VertexBufferStrides, strides.GetPtr(), strides.GetCount());
    m_pDXContext->IASetInputLayout(pVertexDeclarationDX11->GetDXInputLayout());
  }
  else
  {
    m_pDXContext->IASetInputLayout(nullptr);
  }
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

static_assert(W_ARRAY_SIZE(GALTopologyToDX11) == WGALPrimitiveTopology::ENUM_COUNT);

void WGALCommandEncoderImplDX11::SetPrimitiveTopology(WGALPrimitiveTopology::Enum topology)
{
  m_Topology = topology;
}

void WGALCommandEncoderImplDX11::SetBlendState(const WGALBlendState* pBlendState, const WColor& blendFactor, WUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {blendFactor.r, blendFactor.g, blendFactor.b, blendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const WGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void WGALCommandEncoderImplDX11::SetDepthStencilState(const WGALDepthStencilState* pDepthStencilState)
{
  ID3D11DepthStencilState* pDepthStencilStateDX11 = pDepthStencilState != nullptr ? static_cast<const WGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr;
  m_pDXContext->OMSetDepthStencilState(pDepthStencilStateDX11, m_uiStencilRefValue);
}

void WGALCommandEncoderImplDX11::SetRasterizerState(const WGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<const WGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void WGALCommandEncoderImplDX11::SetViewportPlatform(const WRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void WGALCommandEncoderImplDX11::SetScissorRectPlatform(const WRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void WGALCommandEncoderImplDX11::SetStencilReferencePlatform(WUInt8 uiStencilRefValue)
{
  if (m_uiStencilRefValue == uiStencilRefValue)
    return;

  m_uiStencilRefValue = uiStencilRefValue;
  ID3D11DepthStencilState* pState = nullptr;
  m_pDXContext->OMGetDepthStencilState(&pState, nullptr);
  m_pDXContext->OMSetDepthStencilState(pState, m_uiStencilRefValue);
  W_GAL_DX11_RELEASE(pState);
}

//////////////////////////////////////////////////////////////////////////

WResult WGALCommandEncoderImplDX11::DispatchPlatform(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
  return W_SUCCESS;
}

WResult WGALCommandEncoderImplDX11::DispatchIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DispatchIndirect(static_cast<const WGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(WGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, WUInt32 uiStartSlot, WUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case WGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case WGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case WGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case WGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case WGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case WGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(
  WGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, WUInt32 uiStartSlot, WUInt32 uiNumSlots, ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case WGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case WGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case WGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case WGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case WGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case WGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(
  WGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, WUInt32 uiStartSlot, WUInt32 uiNumSlots, ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case WGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case WGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case WGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case WGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case WGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case WGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

// Some state changes are deferred so they can be updated faster
WResult WGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_uiTessellationPatchControlPoints == 0)
  {
    m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[m_Topology.GetValue()]);
  }
  else
  {
    m_pDXContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (m_uiTessellationPatchControlPoints - 1)));
  }

  if (m_BoundVertexBuffersRange.IsValid())
  {
    const WUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const WUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const WUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const WUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((WGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_BoundUnorderedAccessViewsRange.IsValid())
  {
    const WUInt32 uiStartSlot = m_BoundUnorderedAccessViewsRange.m_uiMin;
    const WUInt32 uiNumSlots = m_BoundUnorderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_BoundUnorderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_BoundUnorderedAccessViewsRange.Reset();
  }

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const WUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const WUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((WGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const WUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const WUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((WGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
  return W_SUCCESS;
}

bool WGALCommandEncoderImplDX11::UnsetUnorderedAccessViews(const WGALResourceBase* pResource)
{
  W_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (WUInt32 uiSlot = 0; uiSlot < m_ResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_ResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_ResourcesForUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnorderedAccessViewsRange.SetToIncludeValue(uiSlot);
      bResult = true;
    }
  }

  return bResult;
}
bool WGALCommandEncoderImplDX11::UnsetResourceViews(const WGALResourceBase* pResource)
{
  W_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (WUInt32 uiSlot = 0; uiSlot < m_ResourcesForResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_ResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        m_ResourcesForResourceViews[stage][uiSlot] = nullptr;
        m_pBoundShaderResourceViews[stage][uiSlot] = nullptr;
        m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(uiSlot);
        bResult = true;
      }
    }
  }

  return bResult;
}

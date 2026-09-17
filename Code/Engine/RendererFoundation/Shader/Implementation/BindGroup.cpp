#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

#include <Foundation/Algorithm/HashStream.h>

WUInt64 WGALBindGroupCreationDescription::CalculateHash() const
{
  WHashStreamWriter64 writer;
  writer << m_hBindGroupLayout.GetInternalID().m_Data;
  if (!m_BindGroupItems.IsEmpty())
  {
    auto data = m_BindGroupItems.GetByteArrayPtr();
    writer.WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
  }
  return writer.GetHashValue();
}
void WGALBindGroupCreationDescription::AssertValidDescription(const WGALDevice& m_Device) const
{
  const WGALBindGroupLayout* pLayout = m_Device.GetBindGroupLayout(m_hBindGroupLayout);
  WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  WArrayPtr<const WGALBindGroupItem> items = m_BindGroupItems;
  W_ASSERT_ALWAYS(bindings.GetCount() == items.GetCount(), "Missmatch between bindings and item count");
  const WUInt32 uiBindings = bindings.GetCount();
  for (WUInt32 i = 0; i < uiBindings; ++i)
  {
    const WShaderResourceBinding& binding = bindings[i];
    const WGALBindGroupItem& item = items[i];
    const WBitflags<WGALShaderResourceCategory> category = WGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);

    switch (binding.m_ResourceType)
    {
      case WGALShaderResourceType::Sampler:
      {
        W_ASSERT_ALWAYS(item.m_Flags.IsSet(WGALBindGroupItemFlags::Sampler), "Item type does not match binding");
        const WGALSamplerState* pSampler = m_Device.GetSamplerState(item.m_Sampler.m_hSampler);
        W_ASSERT_ALWAYS(pSampler != nullptr, "Invalid sampler state");
      }
      break;

      case WGALShaderResourceType::ConstantBuffer:
      {
        W_ASSERT_ALWAYS(item.m_Flags.IsSet(WGALBindGroupItemFlags::Buffer), "Item type does not match binding");
        const WGALBuffer* pBuffer = m_Device.GetBuffer(item.m_Buffer.m_hBuffer);
        W_ASSERT_ALWAYS(pBuffer != nullptr, "Invalid buffer");
        W_ASSERT_ALWAYS(item.m_Buffer.m_OverrideTexelBufferFormat == WGALResourceFormat::Invalid, "m_OverrideTexelBufferFormat must be Invalid for constant buffers");
        W_ASSERT_ALWAYS(item.m_Buffer.m_BufferRange.m_uiByteOffset == 0, "Byte offset for constant buffers not supported yet");
        W_ASSERT_ALWAYS(item.m_Buffer.m_BufferRange.m_uiByteCount == pBuffer->GetDescription().m_uiTotalSize, "Byte count for constant buffers not supported yet");
      }
      break;
      case WGALShaderResourceType::TexelBuffer:
      case WGALShaderResourceType::StructuredBuffer:
      case WGALShaderResourceType::ByteAddressBuffer:
      case WGALShaderResourceType::TexelBufferRW:
      case WGALShaderResourceType::StructuredBufferRW:
      case WGALShaderResourceType::ByteAddressBufferRW:
      {

        W_ASSERT_ALWAYS(item.m_Flags.IsSet(WGALBindGroupItemFlags::Buffer), "Item type does not match binding");
        const WGALBuffer* pBuffer = m_Device.GetBuffer(item.m_Buffer.m_hBuffer);
        W_ASSERT_ALWAYS(pBuffer != nullptr, "Invalid buffer");
        const WGALBufferCreationDescription& bufferDesc = pBuffer->GetDescription();
        W_ASSERT_ALWAYS((binding.m_ResourceType == WGALShaderResourceType::TexelBuffer || binding.m_ResourceType == WGALShaderResourceType::TexelBufferRW) || item.m_Buffer.m_OverrideTexelBufferFormat == WGALResourceFormat::Invalid, "m_OverrideTexelBufferFormat must be Invalid for non-texel buffers");
        W_ASSERT_ALWAYS((binding.m_ResourceType != WGALShaderResourceType::TexelBuffer && binding.m_ResourceType != WGALShaderResourceType::TexelBufferRW) || bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::TexelBuffer), "TexelBuffer bindings are only supported on texel buffers");
        W_ASSERT_ALWAYS((binding.m_ResourceType != WGALShaderResourceType::StructuredBuffer && binding.m_ResourceType != WGALShaderResourceType::StructuredBufferRW) || bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::StructuredBuffer), "StructuredBuffer bindings are only supported on structured buffers");
        W_ASSERT_ALWAYS((binding.m_ResourceType != WGALShaderResourceType::ByteAddressBuffer && binding.m_ResourceType != WGALShaderResourceType::ByteAddressBufferRW) || bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::ByteAddressBuffer), "ByteAddressBuffer bindings are only supported on byte address buffers");

        if (category.IsSet(WGALShaderResourceCategory::BufferSRV))
        {
          W_ASSERT_ALWAYS(bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::ShaderResource), "Buffer must have the ShaderResource flag set to be used as an SRV");
        }
        if (category.IsSet(WGALShaderResourceCategory::BufferUAV))
        {
          W_ASSERT_ALWAYS(bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::UnorderedAccess), "Buffer must have the UnorderedAccess flag set to be used as an UAV");
        }

        WUInt32 uiBytesPerElement = 4; // ByteAddress must be multiple of 4
        if (binding.m_ResourceType == WGALShaderResourceType::StructuredBuffer || binding.m_ResourceType == WGALShaderResourceType::StructuredBufferRW)
        {
          uiBytesPerElement = bufferDesc.m_uiStructSize;
        }
        else if (binding.m_ResourceType == WGALShaderResourceType::TexelBuffer || binding.m_ResourceType == WGALShaderResourceType::TexelBufferRW)
        {
          const WGALResourceFormat::Enum viewFormat = item.m_Buffer.m_OverrideTexelBufferFormat == WGALResourceFormat::Invalid ? bufferDesc.m_Format : item.m_Buffer.m_OverrideTexelBufferFormat;
          uiBytesPerElement = WGALResourceFormat::GetBitsPerElement(viewFormat) / 8;
        }

        WGALBufferRange range = item.m_Buffer.m_BufferRange;
        if ((range.m_uiByteOffset % uiBytesPerElement) != 0)
        {
          W_REPORT_FAILURE("m_uiByteOffset {} is not a multiple of the element size {}", range.m_uiByteOffset, uiBytesPerElement);
        }
        if (range.m_uiByteOffset >= bufferDesc.m_uiTotalSize)
        {
          W_REPORT_FAILURE("m_uiByteOffset {} is too big for the buffer of size {}", range.m_uiByteOffset, bufferDesc.m_uiTotalSize);
        }

        if (range.m_uiByteCount != W_GAL_WHOLE_SIZE)
        {
          if ((range.m_uiByteCount % uiBytesPerElement) != 0)
          {
            W_REPORT_FAILURE("m_uiByteCount {} is not a multiple of the element size {}", range.m_uiByteCount, uiBytesPerElement);
          }
          if (range.m_uiByteOffset + range.m_uiByteCount > bufferDesc.m_uiTotalSize)
          {
            W_REPORT_FAILURE("m_uiByteOffset {} + m_uiByteCount {} = {} is too big for the buffer of size {}", range.m_uiByteOffset, range.m_uiByteCount, range.m_uiByteOffset + range.m_uiByteCount, bufferDesc.m_uiTotalSize);
          }
        }
      }
      break;
      case WGALShaderResourceType::Texture:
      case WGALShaderResourceType::TextureRW:
      case WGALShaderResourceType::TextureAndSampler:
      {
        W_ASSERT_ALWAYS(item.m_Flags.IsSet(WGALBindGroupItemFlags::Texture), "Item type does not match binding");
        const WGALTexture* pTexture = m_Device.GetTexture(item.m_Texture.m_hTexture);
        W_ASSERT_ALWAYS(pTexture != nullptr, "Invalid texture");
        const auto& textureDesc = pTexture->GetDescription();

        if (item.m_Texture.m_OverrideViewFormat != WGALResourceFormat::Invalid)
        {
          const WEnum<WGALResourceFormat> format = pTexture->GetDescription().m_Format;
          const WEnum<WGALResourceFormat> overrideFormat = item.m_Texture.m_OverrideViewFormat;
          W_ASSERT_ALWAYS(WGALResourceFormat::GetBitsPerElement(format) == WGALResourceFormat::GetBitsPerElement(overrideFormat), "Format override bits per element ({}) must match the same on the original format ({})", WGALResourceFormat::GetBitsPerElement(overrideFormat), WGALResourceFormat::GetBitsPerElement(format));
          W_ASSERT_ALWAYS(WGALResourceFormat::GetChannelCount(format) == WGALResourceFormat::GetChannelCount(overrideFormat), "Format override channel count ({}) must match the same on the original format ({})", WGALResourceFormat::GetChannelCount(overrideFormat), WGALResourceFormat::GetChannelCount(format));
        }
        // TODO item.m_Texture.m_OverrideTexelBufferFormat
        if (binding.m_ResourceType == WGALShaderResourceType::TextureAndSampler)
        {
          const WGALSamplerState* pSampler = m_Device.GetSamplerState(item.m_Texture.m_hSampler);
          W_ASSERT_ALWAYS(pSampler != nullptr, "Invalid sampler state");
        }

        const WGALTextureRange range = item.m_Texture.m_TextureRange;

        if (!WGALShaderTextureType::IsArray(binding.m_TextureType))
        {
          if (binding.m_TextureType == WGALShaderTextureType::TextureCube)
          {
            W_ASSERT_ALWAYS(range.m_uiArraySlices == 6, "m_uiArraySlices must be 6 for a cube texture binding");
          }
          else
          {
            W_ASSERT_ALWAYS(range.m_uiArraySlices == 1, "m_uiArraySlices must be 1 for non array bindings");
          }
        }

        W_ASSERT_ALWAYS(WGALShaderTextureType::IsMSAA(binding.m_TextureType) == (textureDesc.m_SampleCount != WGALMSAASampleCount::None), "MSAA missmatch between texture and binding");

        if (category.IsSet(WGALShaderResourceCategory::TextureSRV))
        {
          W_ASSERT_ALWAYS(textureDesc.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource), "Texture must have the ShaderResourceView flag set to be used as an SRV");
        }
        if (category.IsSet(WGALShaderResourceCategory::TextureUAV))
        {
          W_ASSERT_ALWAYS(textureDesc.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess), "Texture must have the UnorderedAccess flag set to be used as a UAV");
        }
        const WUInt32 uiSlices = (textureDesc.m_Type == WGALTextureType::TextureCube || textureDesc.m_Type == WGALTextureType::TextureCubeArray) ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
        W_ASSERT_ALWAYS(textureDesc.m_Type != WGALTextureType::Texture2DProxy, "Proxy textures must be resolved to their base texture before binding");
        W_ASSERT_ALWAYS(range.m_uiBaseArraySlice < uiSlices, "Base array slice is out of bounds");
        W_ASSERT_ALWAYS(range.m_uiBaseMipLevel < textureDesc.m_uiMipLevelCount, "Base array slice is out of bounds");
        W_ASSERT_ALWAYS(range.m_uiMipLevels > 0, "Mip level count must be greater than 0");
        W_ASSERT_ALWAYS(range.m_uiArraySlices > 0, "Array slices must be greater than 0");
        W_ASSERT_ALWAYS(range.m_uiMipLevels == W_GAL_ALL_MIP_LEVELS || static_cast<WUInt32>(range.m_uiBaseMipLevel) + range.m_uiMipLevels <= textureDesc.m_uiMipLevelCount, "Mip level range is out of bounds");

        W_ASSERT_ALWAYS(range.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES || static_cast<WUInt32>(range.m_uiBaseArraySlice) + range.m_uiArraySlices <= uiSlices, "Array slice range is out of bounds");
        W_ASSERT_ALWAYS(binding.m_TextureType != WGALShaderTextureType::TextureCube || range.m_uiArraySlices == 6, "Cube textures must have 6 as array slices");
        W_ASSERT_ALWAYS(binding.m_TextureType != WGALShaderTextureType::TextureCubeArray || (range.m_uiArraySlices % 6) == 0, "Cube array textures must have array slices that are multiple of 6");
      }
      break;
      case WGALShaderResourceType::Unknown:
      case WGALShaderResourceType::PushConstants:
      default:
        W_REPORT_FAILURE("Unsupported Shader Resource Type: {}", WArgEnum(binding.m_ResourceType));
        break;
    }
  }
}

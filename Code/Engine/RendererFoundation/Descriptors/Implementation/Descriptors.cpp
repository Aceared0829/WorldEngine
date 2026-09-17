#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

#include "RendererFoundation/Device/Device.h"

#include <RendererFoundation/Resources/ResourceFormats.h>

WResult WGALTextureCreationDescription::Validate(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> initialData) const
{
  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)
  if (m_ResourceAccess.IsImmutable())
  {
    if (m_TextureFlags == WGALTextureUsageFlags::ShaderResource)
    {
      if (initialData.IsEmpty())
      {
        WLog::Error("Trying to create an immutable texture but not supplying initial data");
        return W_FAILURE;
      }
      if (initialData.GetCount() < m_uiMipLevelCount)
      {
        WLog::Error("Trying to create an immutable texture but initialData size '{}' is smaller than mip levels '{}'", initialData.GetCount(), m_uiMipLevelCount);
        return W_FAILURE;
      }
    }
  }

  if (m_uiWidth == 0 || m_uiHeight == 0)
  {
    WLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return W_FAILURE;
  }

  if (m_Type != WGALTextureType::Texture2DArray && m_Type != WGALTextureType::TextureCubeArray)
  {
    if (m_uiArraySize != 1)
    {
      WLog::Error("m_uiArraySize must be 1 for non array textures!");
      return W_FAILURE;
    }
  }

  if (m_Format == WGALResourceFormat::Invalid)
  {
    WLog::Error("Texture format is 'Invalid'");
    return W_FAILURE;
  }

  const auto& caps = pDevice->GetCapabilities();
  WBitflags<WGALResourceFormatSupport> formatSupport = caps.m_FormatSupport[m_Format];
  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget) && !formatSupport.IsSet(WGALResourceFormatSupport::RenderTarget))
  {
    WLog::Error("WGALTextureUsageFlags::RenderTarget not supported on format: {}", WArgEnum(m_Format));
    return W_FAILURE;
  }
  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess) && !formatSupport.IsSet(WGALResourceFormatSupport::TextureRW))
  {
    WLog::Error("WGALTextureUsageFlags::UnorderedAccess not supported on format: {}", WArgEnum(m_Format));
    return W_FAILURE;
  }
  if (m_SampleCount == WGALMSAASampleCount::TwoSamples && !formatSupport.IsSet(WGALResourceFormatSupport::MSAA2x))
  {
    WLog::Error("MSAA 2x not supported on format: {}", WArgEnum(m_Format));
    return W_FAILURE;
  }
  if (m_SampleCount == WGALMSAASampleCount::FourSamples && !formatSupport.IsSet(WGALResourceFormatSupport::MSAA4x))
  {
    WLog::Error("MSAA 4x not supported on format: {}", WArgEnum(m_Format));
    return W_FAILURE;
  }
  if (m_SampleCount == WGALMSAASampleCount::EightSamples && !formatSupport.IsSet(WGALResourceFormatSupport::MSAA8x))
  {
    WLog::Error("MSAA 8x not supported on format: {}", WArgEnum(m_Format));
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WUInt32 WGALTextureCreationDescription::GetNumberOfSlices() const
{
  if (m_Type == WGALTextureType::TextureCube || m_Type == WGALTextureType::TextureCubeArray)
    return m_uiArraySize * 6;
  return m_uiArraySize;
}

WVec3U32 WGALTextureCreationDescription::GetMipMapSize(WUInt32 uiMipLevel) const
{
  WVec3U32 size = {m_uiWidth, m_uiHeight, m_uiDepth};
  size.x = WMath::Max(1u, size.x >> uiMipLevel);
  size.y = WMath::Max(1u, size.y >> uiMipLevel);
  size.z = WMath::Max(1u, size.z >> uiMipLevel);
  return size;
}

WBitflags<WGALResourceState> WGALTextureCreationDescription::GetDefaultState() const
{
  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::Presentable))
    return WGALResourceState::Present;

  bool bDepth = WGALResourceFormat::IsDepthFormat(m_Format);
  if (m_ResourceAccess.IsImmutable())
    return bDepth ? WGALResourceState::DepthStencilRead : WGALResourceState::ShaderResource;

  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource))
    return bDepth ? WGALResourceState::DepthStencilRead : WGALResourceState::ShaderResource;

  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
    return bDepth ? WGALResourceState::DepthStencilWrite : WGALResourceState::RenderTarget;

  if (m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess))
    return WGALResourceState::UnorderedAccess;

  return WGALResourceState::Unknown;
}

WBitflags<WGALResourceState> WGALBufferCreationDescription::GetDefaultState() const
{
  WBitflags<WGALResourceState> state = WGALResourceState::Unknown;
  for (auto flag : m_BufferFlags)
  {
    switch (flag)
    {
      case WGALBufferUsageFlags::VertexBuffer:
        state |= WGALResourceState::VertexBuffer;
        break;
      case WGALBufferUsageFlags::IndexBuffer:
        state |= WGALResourceState::IndexBuffer;
        break;
      case WGALBufferUsageFlags::ConstantBuffer:
        state |= WGALResourceState::ConstantBuffer;
        break;
      case WGALBufferUsageFlags::ShaderResource:
        state |= WGALResourceState::ShaderResource;
        break;
      case WGALBufferUsageFlags::UnorderedAccess:
        state |= WGALResourceState::UnorderedAccess;
        break;
      case WGALBufferUsageFlags::DrawIndirect:
        state |= WGALResourceState::DrawIndirect;
        break;
      default:
        break;
    }
  }

  if (m_ResourceAccess.IsImmutable())
    return state & WGALResourceState::AllReadStates;

  // This is the only write state for buffers. If set, all read states need to be removed to allow creating proper write -> read barriers.
  if (state.IsSet(WGALResourceState::UnorderedAccess))
    state = WGALResourceState::UnorderedAccess;

  return state;
}

WUInt32 WGALBindGroupLayoutCreationDescription::CalculateHash() const
{
  WHashStreamWriter32 writer;
  auto HashBinding = [](WHashStreamWriter32& writer, const WShaderResourceBinding& binding)
  {
    writer << binding.m_ResourceType.GetValue();
    writer << binding.m_TextureType.GetValue();
    writer << binding.m_Stages.GetValue();
    writer << binding.m_iBindGroup;
    writer << binding.m_iSlot;
    writer << binding.m_uiArraySize;
    writer << binding.m_sName;
    if (binding.m_pLayout != nullptr)
    {
      const WShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      writer << pLayout->m_uiTotalSize;
      for (const WShaderConstant& constant : pLayout->m_Constants)
      {
        writer << constant.m_sName;
        writer << constant.m_Type.GetValue();
        writer << constant.m_uiArrayElements;
        writer << constant.m_uiOffset;
      }
    }
  };

  for (const WShaderResourceBinding& binding : m_ResourceBindings)
  {
    HashBinding(writer, binding);
  }
  for (const WShaderResourceBinding& binding : m_ImmutableSamplers)
  {
    HashBinding(writer, binding);
  }
  return writer.GetHashValue();
}

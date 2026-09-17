

#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RendererFallbackResources.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

WUInt32 WBindGroupBuilder::s_uiWrites = 0;
WUInt32 WBindGroupBuilder::s_uiReads = 0;

WBindGroupBuilder::WBindGroupBuilder() = default;

namespace
{
  template <typename T>
  WBitflags<WGALBindGroupItemFlags> GetMetaFlags(const WResourceLock<T>& resource)
  {
    WBitflags<WGALBindGroupItemFlags> metaFlags;
    const bool isFallback = resource.GetAcquireResult() != WResourceAcquireResult::Final;
    const bool isPartiallyLoaded = resource->GetNumQualityLevelsLoadable() != 0;
    if (isFallback)
      metaFlags.Add(WGALBindGroupItemFlags::FallbackResource);
    if (isPartiallyLoaded)
      metaFlags.Add(WGALBindGroupItemFlags::PartiallyLoaded);
    return metaFlags;
  }
} // namespace

void WBindGroupBuilder::ResetBoundResources(const WGALDevice* pDevice)
{
  m_pDevice = pDevice;
  m_bModified = true;
  m_hDefaultSampler = {};
  m_BoundSamplers.Clear();
  m_BoundBuffers.Clear();
  m_BoundTextures.Clear();

  // Platforms that do not support immutable samplers like DX11 still need them to be bound manually, so they are bound here.
  WTempHashedString sLinearSampler("LinearSampler");
  for (auto it : WGALImmutableSamplers::GetImmutableSamplers())
  {
    WGALBindGroupItem item;
    item.m_Flags = WGALBindGroupItemFlags::Sampler;
    item.m_Sampler.m_hSampler = it.Value();
    m_BoundSamplers.Insert(it.Key().GetHash(), item);

    if (it.Key() == sLinearSampler)
    {
      m_hDefaultSampler = it.Value();
    }
  }
  W_ASSERT_DEBUG(!m_hDefaultSampler.IsInvalidated(), "LinearSampler should have been registered at this point.");
}

void WBindGroupBuilder::BindSampler(WTempHashedString sSlotName, WGALSamplerStateHandle hSampler, WBitflags<WGALBindGroupItemFlags> metaFlags)
{
  W_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a reserved sampler name and must not be set manually.");
  W_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a reserved sampler name and must not be set manually.");
  W_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a reserved sampler name and must not be set manually.");
  W_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a reserved sampler name and must not be set manually.");

  if (hSampler.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundSamplers);
    return;
  }

  WGALBindGroupItem item;
  item.m_Flags = (metaFlags & WGALBindGroupItemFlags::MetaFlags) | WGALBindGroupItemFlags::Sampler;
  item.m_Sampler.m_hSampler = hSampler;
  W_ASSERT_DEBUG(m_pDevice->GetSamplerState(item.m_Sampler.m_hSampler) != nullptr, "Invalid sampler handle bound.");

  InsertItem(sSlotName, item, m_BoundSamplers);
}

void WBindGroupBuilder::BindBuffer(WTempHashedString sSlotName, WGALBufferHandle hBuffer, WGALBufferRange bufferRange, WEnum<WGALResourceFormat> overrideTexelBufferFormat, WBitflags<WGALBindGroupItemFlags> metaFlags)
{
  if (hBuffer.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundBuffers);
    return;
  }

  const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  W_ASSERT_DEBUG(pBuffer != nullptr, "Invalid buffer handle bound.");
  WGALBindGroupItem item;
  item.m_Flags = (metaFlags & WGALBindGroupItemFlags::MetaFlags) | WGALBindGroupItemFlags::Buffer;
  item.m_Buffer.m_hBuffer = hBuffer;
  item.m_Buffer.m_BufferRange = pBuffer->ClampRange(bufferRange);
  item.m_Buffer.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  InsertItem(sSlotName, item, m_BoundBuffers);
}

void WBindGroupBuilder::BindTexture(WTempHashedString sSlotName, WGALTextureHandle hTexture, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType, WBitflags<WGALBindGroupItemFlags> metaFlags)
{
  if (hTexture.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundTextures);
    return;
  }

  const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  W_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle bound.");
  // Resolve proxy texture as they only cause pain down the pipeline.
  if (pTexture->GetDescription().m_Type == WGALTextureType::Texture2DProxy)
  {
    const auto pProxy = static_cast<const WGALProxyTexture*>(pTexture);
    hTexture = pProxy->GetParentTextureHandle();
    pTexture = static_cast<const WGALTexture*>(pProxy->GetParentResource());
    textureRange = {pProxy->GetSlice(), 1, 0, W_GAL_ALL_MIP_LEVELS};
  }

  WGALBindGroupItem item;
  item.m_Flags = (metaFlags & WGALBindGroupItemFlags::MetaFlags) | WGALBindGroupItemFlags::Texture;
  item.m_Texture.m_hTexture = hTexture;
  item.m_Texture.m_hSampler = {};
  item.m_Texture.m_TextureRange = pTexture->ClampRange(textureRange);
  item.m_Texture.m_OverrideViewFormat = overrideViewFormat;
  item.m_Texture.m_OverrideViewType = overrideViewType;

  InsertItem(sSlotName, item, m_BoundTextures);
}

void WBindGroupBuilder::BindTexture(WTempHashedString sSlotName, const WTexture2DResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTexture2DResource> pTexture(hTexture, acquireMode);
    WBitflags<WGALBindGroupItemFlags> metaFlags = GetMetaFlags(pTexture);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType, metaFlags);
    BindSampler(sSlotName, pTexture->GetGALSamplerState(), metaFlags);
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void WBindGroupBuilder::BindTexture(WTempHashedString sSlotName, const WTexture3DResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTexture3DResource> pTexture(hTexture, acquireMode);
    WBitflags<WGALBindGroupItemFlags> metaFlags = GetMetaFlags(pTexture);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType, metaFlags);
    BindSampler(sSlotName, pTexture->GetGALSamplerState(), metaFlags);
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void WBindGroupBuilder::BindTexture(WTempHashedString sSlotName, const WTextureCubeResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTextureCubeResource> pTexture(hTexture, acquireMode);
    WBitflags<WGALBindGroupItemFlags> metaFlags = GetMetaFlags(pTexture);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType, metaFlags);
    BindSampler(sSlotName, pTexture->GetGALSamplerState(), metaFlags);
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void WBindGroupBuilder::BindBuffer(WTempHashedString sSlotName, WConstantBufferStorageHandle hBuffer, WGALBufferRange bufferRange, WGALResourceFormat::Enum overrideTexelBufferFormat)
{
  WConstantBufferStorageBase* pStorage = nullptr;
  if (WRenderContext::TryGetConstantBufferStorage(hBuffer, pStorage))
  {
    BindBuffer(sSlotName, pStorage->GetGALBufferHandle(), bufferRange, overrideTexelBufferFormat);
  }
  else
  {
    BindBuffer(sSlotName, WGALBufferHandle(), bufferRange, overrideTexelBufferFormat);
  }
}

void WBindGroupBuilder::CreateBindGroup(WGALBindGroupLayoutHandle hBindGroupLayout, WGALBindGroupCreationDescription& out_bindGroup, WBitflags<WGALBindGroupItemFlags>& out_metaFlags)
{
  out_metaFlags = {};
  const WGALBindGroupLayout* pLayout = m_pDevice->GetBindGroupLayout(hBindGroupLayout);
  W_ASSERT_DEBUG(pLayout != nullptr, "Bind group layout is null.");

  const WArrayPtr<const WShaderResourceBinding> resourceBindings = pLayout->GetDescription().m_ResourceBindings;
  const WUInt32 uiBindings = resourceBindings.GetCount();
  out_bindGroup.m_hBindGroupLayout = hBindGroupLayout;
  out_bindGroup.m_BindGroupItems.Clear();
  out_bindGroup.m_BindGroupItems.Reserve(uiBindings);

  for (WUInt32 i = 0; i < uiBindings; ++i)
  {
    const WShaderResourceBinding& binding = resourceBindings[i];
    WGALBindGroupItem& item = out_bindGroup.m_BindGroupItems.ExpandAndGetRef();

    switch (binding.m_ResourceType)
    {
      case WGALShaderResourceType::Sampler:
      {
        s_uiReads++;
        if (!m_BoundSamplers.TryGetValue(binding.m_sName.GetHash(), item))
        {
          item.m_Flags = WGALBindGroupItemFlags::Sampler | WGALBindGroupItemFlags::EmptyBinding;
          item.m_Sampler.m_hSampler = m_hDefaultSampler;
        }
      }
      break;
      case WGALShaderResourceType::ConstantBuffer:
      case WGALShaderResourceType::TexelBuffer:
      case WGALShaderResourceType::StructuredBuffer:
      case WGALShaderResourceType::ByteAddressBuffer:
      case WGALShaderResourceType::TexelBufferRW:
      case WGALShaderResourceType::StructuredBufferRW:
      case WGALShaderResourceType::ByteAddressBufferRW:
      {
        s_uiReads++;
        if (!m_BoundBuffers.TryGetValue(binding.m_sName.GetHash(), item))
        {
          WGALBufferHandle hBuffer = WGALRendererFallbackResources::GetFallbackBuffer(binding.m_ResourceType);
          W_ASSERT_DEBUG(!hBuffer.IsInvalidated(), "Missing fallback resource for binding resource type {}", WArgEnum(binding.m_ResourceType));
          const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
          item.m_Flags = WGALBindGroupItemFlags::Buffer | WGALBindGroupItemFlags::EmptyBinding;
          item.m_Buffer.m_hBuffer = hBuffer;
          item.m_Buffer.m_BufferRange = pBuffer->ClampRange({});
          item.m_Buffer.m_OverrideTexelBufferFormat = {};
        }
      }
      break;

      case WGALShaderResourceType::Texture:
      case WGALShaderResourceType::TextureRW:
      case WGALShaderResourceType::TextureAndSampler:
      {
        s_uiReads++;
        if (!m_BoundTextures.TryGetValue(binding.m_sName.GetHash(), item))
        {
          const bool bDepth = binding.m_sName.GetString().FindSubString_NoCase("shadow") != nullptr || binding.m_sName.GetString().FindSubString_NoCase("depth");
          WGALTextureHandle hTexture = WGALRendererFallbackResources::GetFallbackTexture(binding.m_ResourceType, binding.m_TextureType, bDepth);
          W_ASSERT_DEBUG(!hTexture.IsInvalidated(), "Missing fallback resource for binding resource type {}, texture type {}, depth {}", WArgEnum(binding.m_ResourceType), WArgEnum(binding.m_TextureType), bDepth);
          const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
          item.m_Flags = WGALBindGroupItemFlags::Texture | WGALBindGroupItemFlags::EmptyBinding;
          item.m_Texture.m_hTexture = hTexture;
          item.m_Texture.m_hSampler = {};
          item.m_Texture.m_TextureRange = pTexture->ClampRange({});
          item.m_Texture.m_OverrideViewFormat = {};
        }

        if (binding.m_ResourceType == WGALShaderResourceType::TextureAndSampler)
        {
          s_uiReads++;
          const WGALBindGroupItem* pSamplerItem = nullptr;
          if (!m_BoundSamplers.TryGetValue(binding.m_sName.GetHash(), pSamplerItem))
          {
            item.m_Flags |= WGALBindGroupItemFlags::EmptyBinding;
            item.m_Texture.m_hSampler = m_hDefaultSampler;
          }
          else
          {
            item.m_Texture.m_hSampler = pSamplerItem->m_Sampler.m_hSampler;
          }
        }

        if (!WGALShaderTextureType::IsArray(binding.m_TextureType))
        {
          item.m_Texture.m_TextureRange.m_uiArraySlices = binding.m_TextureType == WGALShaderTextureType::TextureCube ? 6 : 1;
        }
      }
      break;
      case WGALShaderResourceType::PushConstants:
      case WGALShaderResourceType::Unknown:
      default:
        W_REPORT_FAILURE("Unsupported resource type for binding '{0}'", binding.m_sName, binding.m_ResourceType);
        break;
    }
    out_metaFlags |= (item.m_Flags & WGALBindGroupItemFlags::MetaFlags);
  }
  m_bModified = false;
}

void WBindGroupBuilder::RemoveItem(WTempHashedString sSlotName, WHashTable<WUInt64, WGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  if (ref_Container.Remove(sSlotName.GetHash()))
  {
    m_bModified = true;
    s_uiWrites++;
  }
}

void WBindGroupBuilder::InsertItem(WTempHashedString sSlotName, const WGALBindGroupItem& item, WHashTable<WUInt64, WGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  WGALBindGroupItem oldItem;
  if (ref_Container.Insert(sSlotName.GetHash(), item, &oldItem))
  {
    if (oldItem != item)
    {
      m_bModified = true;
      s_uiWrites++;
    }
  }
  else
  {
    m_bModified = true;
    s_uiWrites++;
  }
}

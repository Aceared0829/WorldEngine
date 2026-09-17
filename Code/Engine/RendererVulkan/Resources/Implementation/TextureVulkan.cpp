#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

vk::Extent3D WGALTextureVulkan::GetMipLevelSize(const WGALTextureCreationDescription& description, WUInt32 uiMipLevel)
{
  vk::Extent3D size = {description.m_uiWidth, description.m_uiHeight, description.m_uiDepth};
  size.width = WMath::Max(1u, size.width >> uiMipLevel);
  size.height = WMath::Max(1u, size.height >> uiMipLevel);
  size.depth = WMath::Max(1u, size.depth >> uiMipLevel);
  return size;
}

vk::ImageSubresourceRange WGALTextureVulkan::GetFullRange() const
{
  vk::ImageSubresourceRange range;
  range.aspectMask = GetAspectMask();
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;
  range.layerCount = m_Description.m_uiArraySize;
  range.levelCount = m_Description.m_uiMipLevelCount;

  if (m_Description.m_Type == WGALTextureType::TextureCube || m_Description.m_Type == WGALTextureType::TextureCubeArray)
  {
    range.layerCount *= 6;
  }

  return range;
}

vk::ImageAspectFlags WGALTextureVulkan::GetAspectMask() const
{
  vk::ImageAspectFlags mask = WConversionUtilsVulkan::IsDepthFormat(m_ImageFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (WConversionUtilsVulkan::IsStencilFormat(m_ImageFormat))
    mask |= vk::ImageAspectFlagBits::eStencil;
  return mask;
}

vk::DescriptorImageInfo WGALTextureVulkan::GetDescriptorImageInfo(WGALTextureRange textureRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALShaderTextureType> textureType, WEnum<WGALResourceFormat> overrideViewFormat) const
{
  vk::DescriptorImageInfo imageInfo;
  View view;
  view.m_TextureRange = textureRange;
  view.m_ResourceType = resourceType;
  view.m_TextureType = textureType;
  view.m_OverrideViewFormat = overrideViewFormat;

  if (!m_TextureViews.TryGetValue(view, imageInfo))
  {
    const WGALResourceFormat::Enum viewFormat = overrideViewFormat == WGALResourceFormat::Invalid ? m_Description.m_Format : overrideViewFormat;

    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.image = m_Image;
    viewCreateInfo.viewType = WConversionUtilsVulkan::GetImageViewType(textureType);
    viewCreateInfo.format = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
    viewCreateInfo.subresourceRange = WConversionUtilsVulkan::GetSubresourceRange(viewFormat, textureRange);
    viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;

    VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &imageInfo.imageView));
    imageInfo.imageLayout = WConversionUtilsVulkan::GetTextureReadLayout(m_ImageFormat);
    if (resourceType == WGALShaderResourceType::TextureRW)
      imageInfo.imageLayout = vk::ImageLayout::eGeneral;
    m_TextureViews.Insert(view, imageInfo);
  }

  return imageInfo;
}

WGALTextureVulkan::WGALTextureVulkan(const WGALTextureCreationDescription& Description)
  : WGALTexture(Description)
{
}

WGALTextureVulkan::~WGALTextureVulkan() = default;

WResult WGALTextureVulkan::InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  m_ImageFormat = ComputeImageFormat(m_pDevice, m_Description.m_Format, createInfo, imageFormats);
  ComputeCreateInfo(m_pDevice, m_Description, createInfo);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    m_Image = static_cast<VkImage>(m_Description.m_pExisitingNativeObject);
    m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);
  }
  else
  {
    WVulkanAllocationCreateInfo allocInfo;
    ComputeAllocInfo(allocInfo);

    vk::ImageFormatProperties props2;
    VK_SUCCEED_OR_RETURN_W_FAILURE(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
    VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_Image, m_pAlloc, &m_AllocInfo));
    m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);
  }
  return W_SUCCESS;
}


vk::Format WGALTextureVulkan::ComputeImageFormat(const WGALDeviceVulkan* pDevice, WEnum<WGALResourceFormat> galFormat, vk::ImageCreateInfo& ref_createInfo, vk::ImageFormatListCreateInfo& ref_imageFormats)
{
  const WGALFormatLookupEntryVulkan& format = pDevice->GetFormatLookupTable().GetFormatInfo(galFormat);

  ref_createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  if (pDevice->GetExtensions().m_bImageFormatList && !format.m_mutableFormats.IsEmpty())
  {
    ref_createInfo.pNext = &ref_imageFormats;

    ref_imageFormats.viewFormatCount = format.m_mutableFormats.GetCount();
    ref_imageFormats.pViewFormats = format.m_mutableFormats.GetData();
  }

  ref_createInfo.format = format.m_format;
  return ref_createInfo.format;
}

void WGALTextureVulkan::ComputeCreateInfo(const WGALDeviceVulkan* pDevice, const WGALTextureCreationDescription& description, vk::ImageCreateInfo& out_createInfo)
{
  W_ASSERT_DEBUG(out_createInfo.format != vk::Format::eUndefined, "No storage format available for given format: {0}", description.m_Format);

  const bool bIsDepth = WConversionUtilsVulkan::IsDepthFormat(out_createInfo.format);

  // Transfer is always needed.
  out_createInfo.initialLayout = vk::ImageLayout::eUndefined;
  out_createInfo.sharingMode = vk::SharingMode::eExclusive;
  out_createInfo.pQueueFamilyIndices = nullptr;
  out_createInfo.queueFamilyIndexCount = 0;
  out_createInfo.tiling = vk::ImageTiling::eOptimal;
  out_createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  // eTransferSrc can be set on everything without any negative effects.
  // #TODO_VULKAN eSampled not needed if we only allow buffer readback or is it necessary for depth?
  out_createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  out_createInfo.extent.width = description.m_uiWidth;
  out_createInfo.extent.height = description.m_uiHeight;
  out_createInfo.extent.depth = description.m_uiDepth;
  out_createInfo.mipLevels = description.m_uiMipLevelCount;

  out_createInfo.samples = static_cast<vk::SampleCountFlagBits>(description.m_SampleCount.GetValue());

  // Shader resources need transfer and sampled support for mip generation via shader.
  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource))
  {
    out_createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
    out_createInfo.usage |= vk::ImageUsageFlagBits::eSampled;
  }
  // VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT
  if (bIsDepth)
  {
    // This looks wrong but apparently, even if you don't intend to render to a depth texture, you need to set the eDepthStencilAttachment flag.
    // VUID-VkImageMemoryBarrier-oldLayout-01210: https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#VUID-VkImageMemoryBarrier-oldLayout-01210
    // If srcQueueFamilyIndex and dstQueueFamilyIndex define a queue family ownership transfer or oldLayout and newLayout define an image layout transition, and oldLayout or newLayout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL then image must have been created with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    out_createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
  }

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
  {
    if (bIsDepth)
    {
    }
    else
    {
      out_createInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
    }
  }

  if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess))
  {
    out_createInfo.usage |= vk::ImageUsageFlagBits::eStorage;
  }

  switch (description.m_Type)
  {
    case WGALTextureType::TextureCube:
    case WGALTextureType::TextureCubeArray:
      out_createInfo.imageType = vk::ImageType::e2D;
      out_createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
      out_createInfo.arrayLayers = description.m_uiArraySize * 6;
      break;

    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DArray:
    case WGALTextureType::Texture2DShared:
    {
      out_createInfo.imageType = vk::ImageType::e2D;
      out_createInfo.arrayLayers = description.m_uiArraySize;
    }
    break;

    case WGALTextureType::Texture3D:
    {
      out_createInfo.arrayLayers = 1;
      out_createInfo.imageType = vk::ImageType::e3D;
      if (description.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
      {
        out_createInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
      }
    }
    break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

void WGALTextureVulkan::ComputeAllocInfo(WVulkanAllocationCreateInfo& ref_allocInfo)
{
  ref_allocInfo.m_usage = WVulkanMemoryUsage::Auto;
}

WUInt32 WGALTextureVulkan::ComputeSubResourceOffsets(const WGALDeviceVulkan* pDevice, const WGALTextureCreationDescription& description, WDynamicArray<SubResourceOffset>& ref_subResourceSizes)
{
  const WUInt32 alignment = (WUInt32)WGALBufferVulkan::GetAlignment(pDevice, vk::BufferUsageFlagBits::eTransferDst);
  const vk::Format stagingFormat = pDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_readback;
  const WUInt8 uiBlockSize = vk::blockSize(stagingFormat);
  const auto blockExtent = vk::blockExtent(stagingFormat);
  const WUInt32 arrayLayers = (description.m_Type == WGALTextureType::TextureCube || description.m_Type == WGALTextureType::TextureCubeArray) ? (description.m_uiArraySize * 6) : description.m_uiArraySize;
  const WUInt32 mipLevels = description.m_uiMipLevelCount;

  ref_subResourceSizes.Reserve(arrayLayers * mipLevels);
  WUInt32 uiOffset = 0;
  for (WUInt32 uiLayer = 0; uiLayer < arrayLayers; uiLayer++)
  {
    for (WUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const WUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      W_ASSERT_DEBUG(ref_subResourceSizes.GetCount() == uiSubresourceIndex, "");

      const vk::Extent3D imageExtent = GetMipLevelSize(description, uiMipLevel);
      const VkExtent3D blockCount = {
        (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
        (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
        (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

      const WUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
      ref_subResourceSizes.PushBack({uiOffset, uiTotalSize, blockCount.width * blockExtent[0], blockCount.height * blockExtent[1]});
      uiOffset += WMemoryUtils::AlignSize(uiTotalSize, alignment);
    }
  }
  return uiOffset;
}

WResult WGALTextureVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  if (m_Image && !m_Description.m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_Image, m_pAlloc);
    m_AllocInfo = {};
  }
  m_Image = nullptr;

  for (auto it : m_TextureViews)
  {
    pVulkanDevice->DeleteLater(it.Value().imageView);
  }
  m_TextureViews.Clear();

  return W_SUCCESS;
}

void WGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_Image, m_pAlloc);
}

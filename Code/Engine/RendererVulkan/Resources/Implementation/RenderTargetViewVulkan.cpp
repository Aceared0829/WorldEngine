#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALRenderTargetViewVulkan::WGALRenderTargetViewVulkan(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description)
  : WGALRenderTargetView(pTexture, Description)
{
}

WGALRenderTargetViewVulkan::~WGALRenderTargetViewVulkan() = default;

WResult WGALRenderTargetViewVulkan::InitPlatform(WGALDevice* pDevice)
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

  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  auto pTextureVulkan = static_cast<const WGALTextureVulkan*>(pTexture->GetParentResource());
  vk::Format vkViewFormat = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;

  if (vkViewFormat == vk::Format::eUndefined)
  {
    WLog::Error("Couldn't get Vulkan format for view!");
    return W_FAILURE;
  }

  vk::Image vkImage = pTextureVulkan->GetImage();

  vk::ImageViewCreateInfo imageViewCreationInfo;
  // The aspect has to follow the actual Vulkan format, not the GAL format: the format table can map a depth-only GAL format onto a combined depth/stencil format when the device lacks the preferred one.
  if (WConversionUtilsVulkan::IsDepthFormat(vkViewFormat))
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (WConversionUtilsVulkan::IsStencilFormat(vkViewFormat))
    {
      imageViewCreationInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  }
  else
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  imageViewCreationInfo.image = vkImage;
  imageViewCreationInfo.format = vkViewFormat;

  const WEnum<WGALTextureType> type = m_Description.m_OverrideViewType != WGALTextureType::Invalid ? m_Description.m_OverrideViewType : texDesc.m_Type;
  if (type == WGALTextureType::Texture2DArray || type == WGALTextureType::TextureCubeArray)
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2DArray;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.layerCount = m_Description.m_uiSliceCount;
  }
  else
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.layerCount = 1;
  }

  m_Range = imageViewCreationInfo.subresourceRange;
  m_bBfullRange = m_Range == pTextureVulkan->GetFullRange();

  VK_SUCCEED_OR_RETURN_W_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&imageViewCreationInfo, nullptr, &m_ImageView));
  pVulkanDevice->SetDebugName("RTV", m_ImageView);
  return W_SUCCESS;
}

WResult WGALRenderTargetViewVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  // Drop the cached framebuffers now rather than when the view is actually freed. The GAL handle of this view can be recycled into a new view before then, which would make the stale cache entry reachable again.
  WResourceCacheVulkan::RenderTargetViewDestroyed(m_ImageView);
  pVulkanDevice->DeleteLater(m_ImageView);
  return W_SUCCESS;
}



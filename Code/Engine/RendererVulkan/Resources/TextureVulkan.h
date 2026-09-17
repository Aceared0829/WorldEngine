#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/Texture.h>

class WGALBufferVulkan;
class WGALDeviceVulkan;

class WGALTextureVulkan : public WGALTexture
{
public:
  struct SubResourceOffset
  {
    W_DECLARE_POD_TYPE();
    WUInt32 m_uiOffset;
    WUInt32 m_uiSize;
    WUInt32 m_uiRowLength;
    WUInt32 m_uiImageHeight;
  };

  static vk::Format ComputeImageFormat(const WGALDeviceVulkan* pDevice, WEnum<WGALResourceFormat> galFormat, vk::ImageCreateInfo& ref_createInfo, vk::ImageFormatListCreateInfo& ref_imageFormats);
  static void ComputeCreateInfo(const WGALDeviceVulkan* pDevice, const WGALTextureCreationDescription& description, vk::ImageCreateInfo& ref_createInfo);
  static void ComputeAllocInfo(WVulkanAllocationCreateInfo& ref_allocInfo);
  static WUInt32 ComputeSubResourceOffsets(const WGALDeviceVulkan* pDevice, const WGALTextureCreationDescription& description, WDynamicArray<SubResourceOffset>& ref_subResourceSizes);
  static vk::Extent3D GetMipLevelSize(const WGALTextureCreationDescription& description, WUInt32 uiMipLevel);

public:
  W_ALWAYS_INLINE vk::Image GetImage() const;
  W_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_ImageFormat; }
  W_ALWAYS_INLINE WVulkanAllocation GetAllocation() const;
  W_ALWAYS_INLINE const WVulkanAllocationInfo& GetAllocationInfo() const;

  vk::Extent3D GetMipLevelSize(WUInt32 uiMipLevel) const { return GetMipLevelSize(m_Description, uiMipLevel); }
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetAspectMask() const;

  vk::DescriptorImageInfo GetDescriptorImageInfo(WGALTextureRange textureRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALShaderTextureType> textureType, WEnum<WGALResourceFormat> overrideViewFormat) const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALTextureVulkan(const WGALTextureCreationDescription& Description);
  ~WGALTextureVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Image m_Image = {};
  vk::Format m_ImageFormat = vk::Format::eUndefined;
  WVulkanAllocation m_pAlloc = nullptr;
  WVulkanAllocationInfo m_AllocInfo;

  // Views
  struct View : WHashableStruct<View>
  {
    WGALTextureRange m_TextureRange;
    WEnum<WGALShaderResourceType> m_ResourceType;
    WEnum<WGALShaderTextureType> m_TextureType;
    WEnum<WGALResourceFormat> m_OverrideViewFormat;

    W_ALWAYS_INLINE static WUInt32 Hash(const View& value) { return value.CalculateHash(); }
    W_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable WHashTable<View, vk::DescriptorImageInfo, View> m_TextureViews;

  WGALDeviceVulkan* m_pDevice = nullptr;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>

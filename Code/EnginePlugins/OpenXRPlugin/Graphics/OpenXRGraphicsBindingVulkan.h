#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT

#  include <RendererVulkan/Device/WVulkanInitInterface.h>

W_DEFINE_AS_POD_TYPE(XrSwapchainImageVulkanKHR);

class WOpenXR;

/// Vulkan implementation of the OpenXR graphics binding.
class W_OPENXRPLUGIN_DLL WOpenXRGraphicsBindingVulkan final : public WOpenXRGraphicsBinding, public WVulkanInitInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WOpenXRGraphicsBindingVulkan, WVulkanInitInterface);

public:
  WOpenXRGraphicsBindingVulkan(WOpenXR* pOpenXR);
  ~WOpenXRGraphicsBindingVulkan();

  // WOpenXRGraphicsBinding
  virtual const char* GetName() const override { return "Vulkan"; }
  virtual XrResult SelectExtension(WDynamicArray<const char*>& extensions, const WDynamicArray<XrExtensionProperties>& extensionProperties) override;
  virtual void LoadFunctionPointers(XrInstance instance) override;
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, WGALDevice* pDevice) override;
  virtual void Deinitialize() override;
  virtual const void* GetGraphicsBinding() const override;
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) override;
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, WUInt32 imageCount, WSizeU32 size, WGALMSAASampleCount::Enum msaaCount, bool bIsDepth, WGALDevice* pDevice, WDynamicArray<WGALTextureHandle>& out_textures) override;
  virtual void CleanupSwapchainImages() override;

  // WVulkanInitInterface
  virtual vk::Instance CreateInstance(const vk::InstanceCreateInfo& createInfo) override;
  virtual vk::PhysicalDevice GetPhysicalDevice(vk::Instance instance) override;
  virtual vk::Device CreateDevice(const vk::DeviceCreateInfo& createInfo) override;

  /// Extends the instance extensions required for OpenXR (for vulkan_enable v1)
  virtual void ExtendInstanceExtensions(const WDynamicArray<vk::ExtensionProperties>& availableExtensions, WDynamicArray<WString>& ref_extensions) override;
  /// Extends the device extensions required for OpenXR (for vulkan_enable v1)
  virtual void ExtendDeviceExtensions(const WDynamicArray<vk::ExtensionProperties>& availableExtensions, WDynamicArray<WString>& ref_extensions) override;

private:
  WOpenXR* m_pOpenXR = nullptr;
  WGALResourceFormat::Enum ConvertTextureFormat(int64_t format) const;

  XrGraphicsBindingVulkanKHR m_GraphicsBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};

  bool m_bUsingVulkanEnable2 = false; // true if using vulkan_enable2, false if using vulkan_enable

  // Function pointers for vulkan_enable2
  PFN_xrCreateVulkanInstanceKHR m_pfnCreateVulkanInstanceKHR = nullptr;
  PFN_xrCreateVulkanDeviceKHR m_pfnCreateVulkanDeviceKHR = nullptr;
  PFN_xrGetVulkanGraphicsDevice2KHR m_pfnGetVulkanGraphicsDevice2KHR = nullptr;
  PFN_xrGetVulkanGraphicsRequirements2KHR m_pfnGetVulkanGraphicsRequirements2KHR = nullptr;

  // Function pointers for vulkan_enable (v1)
  PFN_xrGetVulkanInstanceExtensionsKHR m_pfnGetVulkanInstanceExtensionsKHR = nullptr;
  PFN_xrGetVulkanDeviceExtensionsKHR m_pfnGetVulkanDeviceExtensionsKHR = nullptr;
  PFN_xrGetVulkanGraphicsDeviceKHR m_pfnGetVulkanGraphicsDeviceKHR = nullptr;
  PFN_xrGetVulkanGraphicsRequirementsKHR m_pfnGetVulkanGraphicsRequirementsKHR = nullptr;

  // Vulkan handles created by OpenXR via vulkan_enable2
  vk::Instance m_VulkanInstance;
  vk::PhysicalDevice m_VulkanPhysicalDevice;
  vk::Device m_VulkanDevice;

  // Swapchain images storage
  WHybridArray<XrSwapchainImageVulkanKHR, 3> m_ColorSwapchainImages;
  WHybridArray<XrSwapchainImageVulkanKHR, 3> m_DepthSwapchainImages;
};

#endif

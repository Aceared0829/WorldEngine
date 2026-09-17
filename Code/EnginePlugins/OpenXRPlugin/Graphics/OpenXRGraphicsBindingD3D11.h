#pragma once

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

W_DEFINE_AS_POD_TYPE(XrSwapchainImageD3D11KHR);

/// D3D11 implementation of the OpenXR graphics binding.
class W_OPENXRPLUGIN_DLL WOpenXRGraphicsBindingD3D11 final : public WOpenXRGraphicsBinding
{
public:
  WOpenXRGraphicsBindingD3D11();
  ~WOpenXRGraphicsBindingD3D11();

  virtual const char* GetName() const override { return "D3D11"; }
  virtual XrResult SelectExtension(WDynamicArray<const char*>& extensions, const WDynamicArray<XrExtensionProperties>& extensionProperties) override;
  virtual void LoadFunctionPointers(XrInstance instance) override;
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, WGALDevice* pDevice) override;
  virtual void Deinitialize() override;
  virtual const void* GetGraphicsBinding() const override;
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) override;
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, WUInt32 imageCount, WSizeU32 size, WGALMSAASampleCount::Enum msaaCount, bool bIsDepth, WGALDevice* pDevice, WDynamicArray<WGALTextureHandle>& out_textures) override;
  virtual void CleanupSwapchainImages() override;

private:
  WGALResourceFormat::Enum ConvertTextureFormat(int64_t format) const;

  XrGraphicsBindingD3D11KHR m_GraphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
  PFN_xrGetD3D11GraphicsRequirementsKHR m_pfnGetD3D11GraphicsRequirementsKHR = nullptr;

  // Swapchain images storage
  WHybridArray<XrSwapchainImageD3D11KHR, 3> m_ColorSwapchainImages;
  WHybridArray<XrSwapchainImageD3D11KHR, 3> m_DepthSwapchainImages;
};

#endif // W_ENABLED(W_PLATFORM_WINDOWS)

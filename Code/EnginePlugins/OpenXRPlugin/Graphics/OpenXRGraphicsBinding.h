#pragma once

#include <Foundation/Basics.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class WGALDevice;

/// Abstract interface for graphics API binding in OpenXR.
///
/// This interface abstracts away the graphics API specific code required for OpenXR integration.
class W_OPENXRPLUGIN_DLL WOpenXRGraphicsBinding
{
public:
  virtual ~WOpenXRGraphicsBinding() = default;

  /// Returns the name of the graphics API (e.g., "D3D11", "Vulkan")
  virtual const char* GetName() const = 0;

  /// Adds the required graphics API extension to the list of extensions to enable.
  virtual XrResult SelectExtension(WDynamicArray<const char*>& extensions, const WDynamicArray<XrExtensionProperties>& extensionProperties) = 0;

  /// Loads the graphics API specific OpenXR function pointers.
  virtual void LoadFunctionPointers(XrInstance instance) = 0;

  /// Initializes the graphics binding using the current GAL device.
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, WGALDevice* pDevice) = 0;

  /// Deinitializes the graphics binding.
  virtual void Deinitialize() = 0;

  /// Returns the graphics binding structure to be passed to xrCreateSession.
  virtual const void* GetGraphicsBinding() const = 0;

  /// Selects appropriate swapchain formats for color and depth buffers.
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) = 0;

  /// Creates texture handles from swapchain images.
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, WUInt32 imageCount, WSizeU32 size, WGALMSAASampleCount::Enum msaaCount, bool bIsDepth, WGALDevice* pDevice, WDynamicArray<WGALTextureHandle>& out_textures) = 0;

  /// Cleans up any internal swapchain image storage.
  virtual void CleanupSwapchainImages() = 0;

  /// Creates the appropriate graphics binding for the current platform and GAL device.
  static WUniquePtr<WOpenXRGraphicsBinding> Create(class WOpenXR* pOpenXR, WGALDevice* pDevice);
};

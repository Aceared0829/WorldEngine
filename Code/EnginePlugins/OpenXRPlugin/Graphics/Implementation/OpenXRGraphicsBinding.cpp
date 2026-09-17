#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingD3D11.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingVulkan.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>

WUniquePtr<WOpenXRGraphicsBinding> WOpenXRGraphicsBinding::Create(WOpenXR* pOpenXR, WGALDevice* pDevice)
{
  const WStringView sRenderer = pDevice->GetRenderer();
#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  if (sRenderer == "Vulkan")
  {
    WLog::Info("OpenXR: Creating Vulkan graphics binding");
    return W_DEFAULT_NEW(WOpenXRGraphicsBindingVulkan, pOpenXR);
  }
#endif

#if W_ENABLED(W_PLATFORM_WINDOWS)
  if (sRenderer == "DX11")
  {
    WLog::Info("OpenXR: Creating D3D11 graphics binding");
    return W_DEFAULT_NEW(WOpenXRGraphicsBindingD3D11);
  }
#endif

  WLog::Error("OpenXR: No graphics binding available for renderer '{}'", sRenderer);
  return nullptr;
}

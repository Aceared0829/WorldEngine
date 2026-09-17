#include <RendererVulkan/RendererVulkanPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#endif

#include <Core/System/Window.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Device/WVulkanInitInterface.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackBufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackTextureVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/SharedTextureVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/BindGroupVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/ComputePipelineVulkan.h>
#include <RendererVulkan/State/GraphicsPipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

#if W_ENABLED(W_SUPPORTS_GLFW)
#  include <GLFW/glfw3.h>
#endif

#if W_ENABLED(W_PLATFORM_LINUX) || W_ENABLED(W_PLATFORM_ANDROID)
#  include <errno.h>
#  include <unistd.h>
#endif


W_DEFINE_AS_POD_TYPE(VkLayerProperties);

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
namespace vk
{
  namespace detail
  {
    DispatchLoaderDynamic defaultDispatchLoaderDynamic;
  }
} // namespace vk
#endif

namespace
{
  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
  {
    switch (messageSeverity)
    {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        WLog::Debug("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        WLog::Dev("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        WLog::Info("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        WLog::Error("VK: {}", pCallbackData->pMessage);
        break;
      default:
        break;
    }
    // Only layers are allowed to return true here.
    return VK_FALSE;
  }

  bool isInstanceLayerPresent(const char* szLayerName)
  {
    uint32_t layerCount = 0;
    vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

    WDynamicArray<vk::LayerProperties> availableLayers;
    availableLayers.SetCount(layerCount);
    vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.GetData());

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(szLayerName, layerProperties.layerName) == 0)
      {
        return true;
      }
    }

    return false;
  }

  struct TextureStagingLayout
  {
    WUInt32 m_uiRowPitch = 0;
    WUInt32 m_uiSlicePitch = 0;
    vk::DeviceSize m_uiUploadSize = 0;
    WUInt32 m_uiBufferRowLength = 0;
    WUInt32 m_uiBufferImageHeight = 0;
  };

  TextureStagingLayout ComputeTextureStagingLayout(vk::Format format, const vk::Extent3D& imageExtent, const WGALSystemMemoryDescription& data)
  {
    const WUInt8 uiBlockSize = vk::blockSize(format);
    const auto blockExtent = vk::blockExtent(format);
    const VkExtent3D blockCount = {
      (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
      (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
      (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

    TextureStagingLayout layout;
    const WUInt32 uiTightRowPitch = uiBlockSize * blockCount.width;
    layout.m_uiRowPitch = data.m_uiRowPitch;
    layout.m_uiSlicePitch = data.m_uiSlicePitch != 0 ? data.m_uiSlicePitch : layout.m_uiRowPitch * blockCount.height;

    W_ASSERT_DEV(layout.m_uiRowPitch >= uiTightRowPitch, "Row pitch is smaller than the required image row size.");
    W_ASSERT_DEV((layout.m_uiRowPitch % uiBlockSize) == 0, "Row pitch must be a multiple of the format block size.");
    W_ASSERT_DEV(layout.m_uiSlicePitch >= layout.m_uiRowPitch * blockCount.height, "Slice pitch is smaller than the required image slice size.");
    W_ASSERT_DEV((layout.m_uiSlicePitch % layout.m_uiRowPitch) == 0, "Slice pitch must be a multiple of the row pitch.");

    layout.m_uiUploadSize = static_cast<vk::DeviceSize>(layout.m_uiSlicePitch) * blockCount.depth;
    W_ASSERT_DEV(data.m_pData.GetCount() >= layout.m_uiUploadSize, "Not enough source data provided for the described row and slice pitch.");

    layout.m_uiBufferRowLength = blockExtent[0] * layout.m_uiRowPitch / uiBlockSize;
    layout.m_uiBufferImageHeight = blockExtent[1] * layout.m_uiSlicePitch / layout.m_uiRowPitch;
    return layout;
  }
} // namespace


WInternal::NewInstance<WGALDevice> CreateVulkanDevice(WAllocator* pAllocator, const WGALDeviceCreationDescription& description)
{
  return W_NEW(pAllocator, WGALDeviceVulkan, description);
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, DeviceFactoryVulkan)

ON_CORESYSTEMS_STARTUP
{
  WGALDeviceFactory::RegisterCreatorFunc("Vulkan", &CreateVulkanDevice, "VULKAN", "WShaderCompilerVulkan");
}

ON_CORESYSTEMS_SHUTDOWN
{
  WGALDeviceFactory::UnregisterCreatorFunc("Vulkan");
}

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WGALDeviceVulkan::WGALDeviceVulkan(const WGALDeviceCreationDescription& Description)
  : WGALDevice(Description)
{
}

WGALDeviceVulkan::~WGALDeviceVulkan() = default;

// Init & shutdown functions


vk::Result WGALDeviceVulkan::SelectInstanceExtensions(WDynamicArray<WString>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  WUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
  WDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  W_LOG_BLOCK("InstanceExtensions");
  for (auto& ext : extensionProperties)
  {
    WLog::Debug("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result
  {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop)
      { return WStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SURFACE_EXTENSION_NAME, m_Extensions.m_bSurface));
#if W_ENABLED(W_SUPPORTS_GLFW)
  uint32_t iNumGlfwExtensions = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&iNumGlfwExtensions);
  bool dummy = false;
  for (uint32_t i = 0; i < iNumGlfwExtensions; ++i)
  {
    VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(glfwExtensions[i], dummy));
  }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, m_Extensions.m_bWin32Surface));
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, m_Extensions.m_bAndroidSurface));
#else
#  error "Vulkan platform not supported"
#endif


#if W_ENABLED(W_PLATFORM_LINUX)
  AddExtIfSupported(VK_KHR_XCB_SURFACE_EXTENSION_NAME, m_Extensions.m_bSurfaceXcb);
#endif

  AddExtIfSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_Extensions.m_bDebugUtils);
  m_Extensions.m_bDebugUtilsMarkers = m_Extensions.m_bDebugUtils;

  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, m_Extensions.m_bExternalMemoryCapabilities);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, m_Extensions.m_bExternalSemaphoreCapabilities);
  AddExtIfSupported(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME, m_Extensions.m_bExternalFenceCapabilities);
  AddExtIfSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, m_Extensions.m_bPhysicalDeviceProperties2);
  AddExtIfSupported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, m_Extensions.m_bSurfaceCapabilities2);
  AddExtIfSupported(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME, m_Extensions.m_bSurfaceMaintenance1);

  // Allow OpenXR to extend instance extensions (for vulkan_enable v1)
  if (WVulkanInitInterface* pInitInterface = WSingletonRegistry::GetSingletonInstance<WVulkanInitInterface>())
  {
    pInitInterface->ExtendInstanceExtensions(extensionProperties, extensions);
  }

  return vk::Result::eSuccess;
}


vk::Result WGALDeviceVulkan::SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, WDynamicArray<WString>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  WUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr));
  WDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  W_LOG_BLOCK("DeviceExtensions");
  for (auto& ext : extensionProperties)
  {
    WLog::Debug("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result
  {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop)
      { return WStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    WLog::Warning("Extension '{}' not supported", extensionName);
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, m_Extensions.m_bDeviceSwapChain));
  AddExtIfSupported(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME, m_Extensions.m_bShaderViewportIndexLayer);

  vk::PhysicalDeviceFeatures2 features = GetPhysicalDeviceFeatures(&m_Extensions.m_borderColorEXT);
  m_SupportedStages =
    vk::PipelineStageFlagBits::eTopOfPipe |
    vk::PipelineStageFlagBits::eDrawIndirect |
    vk::PipelineStageFlagBits::eVertexInput |
    vk::PipelineStageFlagBits::eVertexShader |
    vk::PipelineStageFlagBits::eTessellationControlShader |
    vk::PipelineStageFlagBits::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits::eGeometryShader |
    vk::PipelineStageFlagBits::eFragmentShader |
    vk::PipelineStageFlagBits::eEarlyFragmentTests |
    vk::PipelineStageFlagBits::eLateFragmentTests |
    vk::PipelineStageFlagBits::eColorAttachmentOutput |
    vk::PipelineStageFlagBits::eComputeShader |
    vk::PipelineStageFlagBits::eTransfer |
    vk::PipelineStageFlagBits::eBottomOfPipe |
    vk::PipelineStageFlagBits::eHost |
    vk::PipelineStageFlagBits::eAllGraphics |
    vk::PipelineStageFlagBits::eAllCommands;

  if (!features.features.geometryShader)
  {
    m_SupportedStages &= ~vk::PipelineStageFlags(vk::PipelineStageFlagBits::eGeometryShader);
    WLog::Warning("Geometry shaders are not supported.");
    m_UnsupportedStages |= vk::PipelineStageFlagBits::eGeometryShader;
  }

  if (!features.features.tessellationShader)
  {
    m_SupportedStages &= ~vk::PipelineStageFlags(vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader);
    WLog::Warning("Tessellation shaders are not supported.");
    m_UnsupportedStages |= vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  }

  // Only use the extension if it allows us to not specify a format or we would need to create different samplers for every texture.
  if (m_Extensions.m_borderColorEXT.customBorderColors && m_Extensions.m_borderColorEXT.customBorderColorWithoutFormat)
  {
    AddExtIfSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, m_Extensions.m_bBorderColorFloat);
    if (m_Extensions.m_bBorderColorFloat)
    {
      m_Extensions.m_borderColorEXT.pNext = const_cast<void*>(deviceCreateInfo.pNext);
      deviceCreateInfo.pNext = &m_Extensions.m_borderColorEXT;
    }
    else
    {
      WLog::Warning("Custom border color samplers are not supported.");
    }
  }

  AddExtIfSupported(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME, m_Extensions.m_bImageFormatList);

  // The extension has no feature struct that would need to be chained into the device, its presence is enough.
  AddExtIfSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, m_Extensions.m_bConservativeRasterization);

  AddExtIfSupported(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, m_Extensions.m_bTimelineSemaphore);

  if (m_Extensions.m_bTimelineSemaphore)
  {
    m_Extensions.m_timelineSemaphoresEXT.pNext = const_cast<void*>(deviceCreateInfo.pNext);
    deviceCreateInfo.pNext = &m_Extensions.m_timelineSemaphoresEXT;
    m_Extensions.m_timelineSemaphoresEXT.timelineSemaphore = true;
  }

  AddExtIfSupported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, m_Extensions.m_bSynchronization2);
  if (m_Extensions.m_bSynchronization2)
  {
    m_Extensions.m_synchronization2Features.pNext = const_cast<void*>(deviceCreateInfo.pNext);
    deviceCreateInfo.pNext = &m_Extensions.m_synchronization2Features;
    m_Extensions.m_synchronization2Features.synchronization2 = true;
  }


  AddExtIfSupported(VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, m_Extensions.m_bSwapchainMaintenance1);
  if (m_Extensions.m_bSwapchainMaintenance1)
  {
    m_Extensions.m_swapchainMaintenance1Features.pNext = const_cast<void*>(deviceCreateInfo.pNext);
    deviceCreateInfo.pNext = &m_Extensions.m_swapchainMaintenance1Features;
    m_Extensions.m_swapchainMaintenance1Features.swapchainMaintenance1 = true;
  }
  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, m_Extensions.m_bExternalMemory);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME, m_Extensions.m_bExternalSemaphore);
#if W_ENABLED(W_PLATFORM_LINUX) || W_ENABLED(W_PLATFORM_ANDROID)
  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, m_Extensions.m_bExternalMemoryFd);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, m_Extensions.m_bExternalSemaphoreFd);
#elif W_ENABLED(W_PLATFORM_WINDOWS)
  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, m_Extensions.m_bExternalMemoryWin32);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, m_Extensions.m_bExternalSemaphoreWin32);
#endif

  // Allow OpenXR to extend device extensions (for vulkan_enable v1)
  if (WVulkanInitInterface* pInitInterface = WSingletonRegistry::GetSingletonInstance<WVulkanInitInterface>())
  {
    pInitInterface->ExtendDeviceExtensions(extensionProperties, extensions);
  }

  return vk::Result::eSuccess;
}

WStringView WGALDeviceVulkan::GetRendererPlatform()
{
  return "Vulkan";
}

WResult WGALDeviceVulkan::InitPlatform()
{
  W_LOG_BLOCK("WGALDeviceVulkan::InitPlatform");

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
  vk::detail::defaultDispatchLoaderDynamic.init();
#endif
  WVulkanInitInterface* pInitInterface = WSingletonRegistry::GetSingletonInstance<WVulkanInitInterface>();
  const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
  {
    // Create instance
    // We require Vulkan 1.1 because of three features:
    // 1. Descriptor set pools return vk::Result::eErrorOutOfPoolMemory if exhausted. Removing the requirement to count usage yourself.
    // 2. Viewport height can be negative which performs y-inversion of the clip-space to framebuffer-space transform.
    // 3. Vulkan 1.0 is a pain to work with.
    vk::ApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_1;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // TODO put WorldEngine version here
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);      // TODO put WorldEngine version here
    applicationInfo.pApplicationName = "WorldEngine";
    applicationInfo.pEngineName = "WorldEngine";

    WHybridArray<WString, 8> instanceExtensions;
    VK_SUCCEED_OR_RETURN_W_FAILURE(SelectInstanceExtensions(instanceExtensions));
    WHybridArray<const char*, 6> instanceExtensionsPtr;
    for (const WString& ext : instanceExtensions)
      instanceExtensionsPtr.PushBack(ext.GetData());

    vk::InstanceCreateInfo instanceCreateInfo;
    // enabling support for win32 surfaces
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount = instanceExtensionsPtr.GetCount();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionsPtr.GetData();

    instanceCreateInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_Description.m_bDebugDevice)
    {
      debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = debugCallback;
      debugCreateInfo.pUserData = nullptr;

      if (isInstanceLayerPresent(layers[0]))
      {
        instanceCreateInfo.enabledLayerCount = W_ARRAY_SIZE(layers);
        instanceCreateInfo.ppEnabledLayerNames = layers;
      }
      else
      {
        WLog::Dev("The Khronos validation layer is not supported on this device. Will run without validation layer.");
      }

      if (m_Extensions.m_bDebugUtils)
      {
        debugCreateInfo.pNext = instanceCreateInfo.pNext;
        instanceCreateInfo.pNext = &debugCreateInfo;
      }

      // Comment out if to force enable synchronization validation on any platform.
      if (false)
      {
        const char* layer_name = "VK_LAYER_KHRONOS_validation";

        const VkBool32 setting_validate_core = VK_TRUE;
        const VkBool32 setting_validate_sync = VK_TRUE;
        const VkBool32 setting_thread_safety = VK_TRUE;
        const char* setting_debug_action[] = {"VK_DBG_LAYER_ACTION_LOG_MSG"};
        const char* setting_report_flags[] = {"info", "warn", "perf", "error", "debug"};
        const VkBool32 setting_enable_message_limit = VK_TRUE;
        const int32_t setting_duplicate_message_limit = 3;

        const VkLayerSettingEXT settings[] = {
          {layer_name, "sync_queue_submit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_sync},
          {layer_name, "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_core},
          {layer_name, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_sync},
          {layer_name, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_thread_safety},
          {layer_name, "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, 1, setting_debug_action},
          {layer_name, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, W_ARRAY_SIZE(setting_report_flags), setting_report_flags},
          {layer_name, "enable_message_limit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_enable_message_limit},
          {layer_name, "duplicate_message_limit", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &setting_duplicate_message_limit}};

        VkLayerSettingsCreateInfoEXT layer_settings_create_info = {
          VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, W_ARRAY_SIZE(settings), settings};

        {
          layer_settings_create_info.pNext = instanceCreateInfo.pNext;
          instanceCreateInfo.pNext = &layer_settings_create_info;
        }
      }
    }

    if (pInitInterface)
    {
      m_Instance = pInitInterface->CreateInstance(instanceCreateInfo);
    }
    if (!m_Instance)
    {
      m_Instance = vk::createInstance(instanceCreateInfo);
    }
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
    vk::detail::defaultDispatchLoaderDynamic.init(m_Instance);
#endif
    m_DispatchContext.InitInstance(m_Instance, &m_Extensions);
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (m_Extensions.m_bDebugUtils)
    {
      vk::DebugUtilsMessengerCreateInfoEXT createInfo(debugCreateInfo);
      m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_DispatchContext);
    }
#endif

    if (!m_Instance)
    {
      WLog::Error("Failed to create Vulkan instance!");
      return W_FAILURE;
    }
  }

  if (pInitInterface)
  {
    m_PhysicalDevice = pInitInterface->GetPhysicalDevice(m_Instance);
  }
  if (!m_PhysicalDevice)
  {
    // physical device
    WUInt32 physicalDeviceCount = 0;
    WHybridArray<vk::PhysicalDevice, 2> physicalDevices;
    VK_SUCCEED_OR_RETURN_W_FAILURE(m_Instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0)
    {
      WLog::Error("No available physical device to create a Vulkan device on!");
      return W_FAILURE;
    }

    physicalDevices.SetCount(physicalDeviceCount);
    VK_SUCCEED_OR_RETURN_W_FAILURE(m_Instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.GetData()));

    // TODO choosable physical device?
    // TODO making sure we have a hardware device?
    m_PhysicalDevice = physicalDevices[0];
  }
  {
    m_Properties = m_PhysicalDevice.getProperties2();
    WLog::Dev("Selected physical device \"{}\" for device creation.", m_Properties.properties.deviceName);

    // This is a workaround for broken lavapipe drivers which cannot handle label scopes that span across multiple command buffers.
    WStringBuilder sDeviceName = WStringUtf8(m_Properties.properties.deviceName).GetView();
    if (sDeviceName.FindSubString_NoCase("LLVMPIPE") != nullptr)
    {
      m_Extensions.m_bDebugUtilsMarkers = false;
    }

    {
      vk::PhysicalDeviceSynchronization2Features sync2Features;
      vk::PhysicalDeviceTimelineSemaphoreFeatures timelineFeatures;
      sync2Features.pNext = &timelineFeatures;
      vk::PhysicalDeviceFeatures2 features2 = GetPhysicalDeviceFeatures(&sync2Features);
      m_Extensions.m_bTimelineSemaphore = static_cast<bool>(timelineFeatures.timelineSemaphore);
      m_Extensions.m_timelineSemaphoresEXT = timelineFeatures;
      m_Extensions.m_bSynchronization2 = static_cast<bool>(sync2Features.synchronization2);
      m_Extensions.m_synchronization2Features = sync2Features;
    }
  }

  WHybridArray<vk::QueueFamilyProperties, 4> queueFamilyProperties;
  {
    // Device
    WUInt32 queueFamilyPropertyCount = 0;
    m_PhysicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
    if (queueFamilyPropertyCount == 0)
    {
      WLog::Error("No available device queues on physical device!");
      return W_FAILURE;
    }
    queueFamilyProperties.SetCount(queueFamilyPropertyCount);
    m_PhysicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyProperties.GetData());

    {
      W_LOG_BLOCK("Queue Families");
      for (WUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
      {
        const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
        WLog::Debug("Queue count: {}, flags: {}", queueFamilyProperty.queueCount, vk::to_string(queueFamilyProperty.queueFlags).data());
      }
    }

    // Select best queue family for graphics and transfers.
    for (WUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
    {
      const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
      if (queueFamilyProperty.queueCount == 0)
        continue;
      constexpr auto graphicsFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
      if ((queueFamilyProperty.queueFlags & graphicsFlags) == graphicsFlags)
      {
        m_GraphicsQueue.m_uiQueueFamily = i;
      }
      if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eTransfer)
      {
        if (m_TransferQueue.m_uiQueueFamily == -1)
        {
          m_TransferQueue.m_uiQueueFamily = i;
        }
        else if ((queueFamilyProperty.queueFlags & graphicsFlags) == vk::QueueFlagBits())
        {
          // Prefer a queue that can't be used for graphics.
          m_TransferQueue.m_uiQueueFamily = i;
        }
      }
    }
    if (m_GraphicsQueue.m_uiQueueFamily == -1)
    {
      WLog::Error("No graphics queue found.");
      return W_FAILURE;
    }
    if (m_TransferQueue.m_uiQueueFamily == -1)
    {
      WLog::Warning("No transfer queue found.");
    }

    constexpr float queuePriority = 0.f;

    WHybridArray<vk::DeviceQueueCreateInfo, 2> queues;

    vk::DeviceQueueCreateInfo& graphicsQueueCreateInfo = queues.ExpandAndGetRef();
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.queueFamilyIndex = m_GraphicsQueue.m_uiQueueFamily;

    if (m_GraphicsQueue.m_uiQueueFamily != m_TransferQueue.m_uiQueueFamily && m_TransferQueue.m_uiQueueFamily != -1)
    {
      vk::DeviceQueueCreateInfo& transferQueueCreateInfo = queues.ExpandAndGetRef();
      transferQueueCreateInfo.pQueuePriorities = &queuePriority;
      transferQueueCreateInfo.queueCount = 1;
      transferQueueCreateInfo.queueFamilyIndex = m_TransferQueue.m_uiQueueFamily;
    }

    vk::DeviceCreateInfo deviceCreateInfo = {};
    WHybridArray<WString, 6> deviceExtensions;
    VK_SUCCEED_OR_RETURN_W_FAILURE(SelectDeviceExtensions(deviceCreateInfo, deviceExtensions));
    WHybridArray<const char*, 6> deviceExtensionsPtr;
    for (const WString& ext : deviceExtensions)
      deviceExtensionsPtr.PushBack(ext.GetData());

    deviceCreateInfo.enabledExtensionCount = deviceExtensionsPtr.GetCount();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionsPtr.GetData();

    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures = GetPhysicalDeviceFeatures();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures.features; // Enabling all available features for now
    deviceCreateInfo.queueCreateInfoCount = queues.GetCount();
    deviceCreateInfo.pQueueCreateInfos = queues.GetData();

    if (pInitInterface)
    {
      m_Device = pInitInterface->CreateDevice(deviceCreateInfo);
    }
    if (!m_Device)
    {
      VK_SUCCEED_OR_RETURN_W_FAILURE(m_PhysicalDevice.createDevice(&deviceCreateInfo, nullptr, &m_Device));
    }
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
    vk::detail::defaultDispatchLoaderDynamic.init(m_Device);
#endif
    m_Device.getQueue(m_GraphicsQueue.m_uiQueueFamily, m_GraphicsQueue.m_uiQueueIndex, &m_GraphicsQueue.m_queue);

    if (m_GraphicsQueue.m_uiQueueFamily != m_TransferQueue.m_uiQueueFamily && m_TransferQueue.m_uiQueueFamily != -1)
    {
      m_Device.getQueue(m_TransferQueue.m_uiQueueFamily, m_TransferQueue.m_uiQueueIndex, &m_TransferQueue.m_queue);
    }

    m_DispatchContext.InitDevice(m_Device, &m_Extensions);
  }

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
  VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::Initialize(m_PhysicalDevice, m_Device, m_Instance, vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr, vk::detail::defaultDispatchLoaderDynamic.vkGetDeviceProcAddr));
#else
  VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::Initialize(m_PhysicalDevice, m_Device, m_Instance, vkGetInstanceProcAddr, vkGetDeviceProcAddr));
#endif
  m_MemoryProperties = m_PhysicalDevice.getMemoryProperties();

  // Fill lookup table
  FillFormatLookupTable();

  WClipSpaceDepthRange::Default = WClipSpaceDepthRange::ZeroToOne;
  // We use WClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  WClipSpaceYMode::RenderToTextureDefault = WClipSpaceYMode::Regular;

  m_pCommandBufferPool = W_NEW(&m_Allocator, WCommandBufferPoolVulkan, &m_Allocator);
  m_pCommandBufferPool->Initialize(m_Device, m_GraphicsQueue.m_uiQueueFamily);
  m_pStagingBufferPool = W_NEW(&m_Allocator, WStagingBufferPoolVulkan);
  // This instance is only utilized when using WGALUpdateMode::CopyToTempStorage. All other updates run in the instance of the WInitContextVulkan so we don't expect a lot of memory to be used here.
  m_pStagingBufferPool->Initialize(this, 1 * 1024 * 1024);
  m_pQueryPool = W_NEW(&m_Allocator, WQueryPoolVulkan, this);
  m_pQueryPool->Initialize(queueFamilyProperties[m_GraphicsQueue.m_uiQueueFamily].timestampValidBits);
  m_pFenceQueue = W_NEW(&m_Allocator, WFenceQueueVulkan, this);
  m_pInitContext = W_NEW(&m_Allocator, WInitContextVulkan, this);

  WSemaphorePoolVulkan::Initialize(m_Device);
  WFencePoolVulkan::Initialize(m_Device);
  WResourceCacheVulkan::Initialize(this, m_Device);
  WTransientDescriptorSetPoolVulkan::Initialize(m_Device);
  WDescriptorSetPoolVulkan::Initialize(this);

  m_pCommandEncoderImpl = W_NEW(&m_Allocator, WGALCommandEncoderImplVulkan, *this);
  m_pCommandEncoder = W_NEW(&m_Allocator, WGALCommandEncoder, *this, *m_pCommandEncoderImpl);

  WGALWindowSwapChain::SetFactoryMethod([this](const WGALWindowSwapChainCreationDescription& desc) -> WGALSwapChainHandle
    { return CreateSwapChain([this, &desc](WAllocator* pAllocator) -> WGALSwapChain*
        { return W_NEW(pAllocator, WGALSwapChainVulkan, desc); }); });

  return W_SUCCESS;
}

void WGALDeviceVulkan::SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, WVulkanAllocation pAllocation)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (m_Extensions.m_bDebugUtils)
  {
    m_Device.setDebugUtilsObjectNameEXT(info, m_DispatchContext);
  }
  if (pAllocation)
    WMemoryAllocatorVulkan::SetAllocationUserData(pAllocation, info.pObjectName);
#endif
}

void WGALDeviceVulkan::ReportLiveGpuObjects()
{
  // This is automatically done in the validation layer and can't be easily done manually.
}

void WGALDeviceVulkan::UploadBufferStaging(WGALDeviceVulkan& ref_device, WStagingBufferPoolVulkan* pStagingBufferPool, vk::CommandBuffer commandBuffer, const WGALBufferVulkan* pBuffer, WArrayPtr<const WUInt8> initialData, vk::DeviceSize dstOffset)
{
  // #TODO_VULKAN Use transfer queue
  WStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(initialData.GetCount());
  WMemoryUtils::Copy(stagingBuffer.m_Data.GetPtr(), initialData.GetPtr(), initialData.GetCount());

  vk::BufferCopy region;
  region.srcOffset = stagingBuffer.m_uiOffset;
  region.dstOffset = dstOffset;
  region.size = initialData.GetCount();

  WBarrierUtilsVulkan barriers(ref_device, commandBuffer);

  barriers.BufferBarrier(stagingBuffer.m_buffer,
    WGALResourceState::CpuWrite, WGALResourceState::CopySource);

  // #TODO_VULKAN atomic min size violation?
  commandBuffer.copyBuffer(stagingBuffer.m_buffer, pBuffer->GetVkBuffer(), 1, &region);
}

void WGALDeviceVulkan::UploadTextureStaging(WGALDeviceVulkan& ref_device, WStagingBufferPoolVulkan* pStagingBufferPool, vk::CommandBuffer commandBuffer, const WGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const vk::Offset3D& imageOffset, const vk::Extent3D& imageExtent, const WGALSystemMemoryDescription& data)
{
  const vk::ImageSubresourceRange subresourceRange = WConversionUtilsVulkan::GetSubresourceRange(subResource);

  WBarrierUtilsVulkan barriers(ref_device, commandBuffer);

  for (WUInt32 i = 0; i < subResource.layerCount; i++)
  {
    const vk::Format format = pTexture->GetImageFormat();
    const TextureStagingLayout layout = ComputeTextureStagingLayout(format, imageExtent, data);
    W_ASSERT_DEV(data.m_pData.GetCount() >= (i + 1) * layout.m_uiUploadSize, "Not enough source data provided for the requested texture array layers.");
    auto pLayerData = data.m_pData.GetPtr() + i * layout.m_uiUploadSize;

    WStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(layout.m_uiUploadSize);
    WMemoryUtils::Copy(stagingBuffer.m_Data.GetPtr(), pLayerData, layout.m_uiUploadSize);

    vk::BufferImageCopy region = {};
    region.imageSubresource = subResource;
    region.imageOffset = imageOffset;
    region.imageExtent = imageExtent;

    region.bufferOffset = stagingBuffer.m_uiOffset;
    region.bufferRowLength = layout.m_uiBufferRowLength;
    region.bufferImageHeight = layout.m_uiBufferImageHeight;

    barriers.BufferBarrier(stagingBuffer.m_buffer,
      WGALResourceState::CpuWrite, WGALResourceState::CopySource);

    // #TODO_VULKAN atomic min size violation?
    commandBuffer.copyBufferToImage(stagingBuffer.m_buffer, pTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &region);
  }
}

WResult WGALDeviceVulkan::ShutdownPlatform()
{
  DestroyDeadObjects();

  WGALWindowSwapChain::SetFactoryMethod({});
  if (m_LastCommandBufferFinished)
    ReclaimLater(m_LastCommandBufferFinished, m_pCommandBufferPool.Borrow());

  // We couldn't create a device in the first place, so early out of shutdown
  if (!m_Device)
  {
    return W_SUCCESS;
  }

  WaitIdleInternal(true);

  m_pStagingBufferPool->DeInitialize();
  m_pStagingBufferPool = nullptr;
  m_pCommandEncoder = nullptr;
  m_pCommandEncoderImpl = nullptr;
  m_pCommandBufferPool->DeInitialize();
  m_pCommandBufferPool = nullptr;

  m_pQueryPool->DeInitialize();
  m_pQueryPool = nullptr;
  m_pFenceQueue = nullptr;
  m_pInitContext = nullptr;

  WSemaphorePoolVulkan::DeInitialize();
  WFencePoolVulkan::DeInitialize();
  WResourceCacheVulkan::DeInitialize();
  WDescriptorSetPoolVulkan::DeInitialize();
  WTransientDescriptorSetPoolVulkan::DeInitialize();
  WMemoryAllocatorVulkan::DeInitialize();

  m_Device.waitIdle();
  m_Device.destroy();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (m_Extensions.m_bDebugUtils && m_DispatchContext.vkDestroyDebugUtilsMessengerEXT != nullptr)
  {
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_DispatchContext);
  }
#endif

  m_Instance.destroy();
  ReportLiveGpuObjects();

  return W_SUCCESS;
}

// Pipeline & Pass functions

vk::CommandBuffer& WGALDeviceVulkan::GetCurrentCommandBuffer()
{
  vk::CommandBuffer& commandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
  if (!commandBuffer)
  {
    // Restart new command buffer if none is active already.
    commandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    VK_ASSERT_DEBUG(commandBuffer.begin(&beginInfo));

    m_pCommandEncoderImpl->SetCurrentCommandBuffer(&commandBuffer);
  }
  return commandBuffer;
}

WQueryPoolVulkan& WGALDeviceVulkan::GetQueryPool() const
{
  return *m_pQueryPool.Borrow();
}

WFenceQueueVulkan& WGALDeviceVulkan::GetFenceQueue() const
{
  return *m_pFenceQueue.Borrow();
}

WStagingBufferPoolVulkan& WGALDeviceVulkan::GetStagingBufferPool() const
{
  return *m_pStagingBufferPool.Borrow();
}

WInitContextVulkan& WGALDeviceVulkan::GetInitContext() const
{
  return *m_pInitContext.Borrow();
}

WGALTextureHandle WGALDeviceVulkan::CreateTextureInternal(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData)
{
  WGALTextureVulkan* pTexture = W_NEW(&m_Allocator, WGALTextureVulkan, description);

  if (!pTexture->InitPlatform(this, initialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pTexture);
    return WGALTextureHandle();
  }

  return FinalizeTextureInternal(description, pTexture);
}

WGALBufferHandle WGALDeviceVulkan::CreateBufferInternal(const WGALBufferCreationDescription& description, WArrayPtr<const WUInt8> initialData)
{
  WGALBufferVulkan* pBuffer = W_NEW(&m_Allocator, WGALBufferVulkan, description);

  if (!pBuffer->InitPlatform(this, initialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pBuffer);
    return WGALBufferHandle();
  }

  WGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));
  return hBuffer;
}

vk::Fence WGALDeviceVulkan::Submit(bool bAddSignalSemaphore, bool bAddUpdateForNextFrameCommands)
{
  m_pCommandEncoderImpl->BeforeCommandBufferSubmit();
  m_pStagingBufferPool->BeforeCommandBufferSubmit();
  if (bAddUpdateForNextFrameCommands)
  {
    m_pInitContext->ExecutePendingCopies(m_PendingBufferCopies, m_PendingTextureCopies);
    m_PendingBufferCopies.Clear();
    m_PendingTextureCopies.Clear();
  }
  vk::CommandBuffer initCommandBuffer = m_pInitContext->GetFinishedCommandBuffer();

  bool bHasCmdBuffer = initCommandBuffer || m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;

  WHybridArray<vk::CommandBuffer, 3> buffers;
  vk::SubmitInfo submitInfo = {};
  if (bHasCmdBuffer)
  {
    vk::CommandBuffer mainCommandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
    if (initCommandBuffer)
    {
      // Any background loading that happened up to this point needs to be submitted first.
      // The main render command buffer assumes that all new resources are in their default state which is made sure by submitting this command buffer.
      buffers.PushBack(initCommandBuffer);
    }
    if (mainCommandBuffer)
    {
      mainCommandBuffer.end();
      buffers.PushBack(mainCommandBuffer);
    }
    submitInfo.commandBufferCount = buffers.GetCount();
    submitInfo.pCommandBuffers = buffers.GetData();
  }

  if (m_LastCommandBufferFinished)
  {
    AddWaitSemaphore(WGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(m_LastCommandBufferFinished, vk::PipelineStageFlagBits::eAllCommands));
    ReclaimLater(m_LastCommandBufferFinished);
  }

  if (bAddSignalSemaphore)
  {
    m_LastCommandBufferFinished = WSemaphorePoolVulkan::RequestSemaphore();
    AddSignalSemaphore(WGALDeviceVulkan::SemaphoreInfo::MakeSignalSemaphore(m_LastCommandBufferFinished));
  }
  vk::Fence renderFence = WFencePoolVulkan::RequestFence();

  WHybridArray<vk::Semaphore, 3> waitSemaphores;
  WHybridArray<vk::PipelineStageFlags, 3> waitStages;
  WHybridArray<vk::Semaphore, 3> signalSemaphores;

  WHybridArray<WUInt64, 3> waitSemaphoreValues;
  WHybridArray<WUInt64, 3> signalSemaphoreValues;
  for (const SemaphoreInfo sem : m_WaitSemaphores)
  {
    waitSemaphores.PushBack(sem.m_semaphore);
    if (sem.m_type == vk::SemaphoreType::eTimeline)
    {
      waitSemaphoreValues.PushBack(sem.m_uiValue);
    }
    waitStages.PushBack(vk::PipelineStageFlagBits::eAllCommands);
  }
  m_WaitSemaphores.Clear();

  for (const SemaphoreInfo sem : m_SignalSemaphores)
  {
    signalSemaphores.PushBack(sem.m_semaphore);
    if (sem.m_type == vk::SemaphoreType::eTimeline)
    {
      signalSemaphoreValues.PushBack(sem.m_uiValue);
    }
  }
  m_SignalSemaphores.Clear();


  // If a timeline semaphore is present, all semaphores need a value, even binary ones because validation says so.
  if (waitSemaphoreValues.GetCount() > 0)
  {
    waitSemaphoreValues.SetCount(waitSemaphores.GetCount());
  }
  if (signalSemaphoreValues.GetCount() > 0)
  {
    signalSemaphoreValues.SetCount(signalSemaphores.GetCount());
  }

  vk::TimelineSemaphoreSubmitInfo timelineInfo;
  timelineInfo.waitSemaphoreValueCount = waitSemaphoreValues.GetCount();
  static_assert(sizeof(WUInt64) == sizeof(uint64_t));
  timelineInfo.pWaitSemaphoreValues = reinterpret_cast<const uint64_t*>(waitSemaphoreValues.GetData());
  timelineInfo.signalSemaphoreValueCount = signalSemaphoreValues.GetCount();
  timelineInfo.pSignalSemaphoreValues = reinterpret_cast<const uint64_t*>(signalSemaphoreValues.GetData());

  if (timelineInfo.waitSemaphoreValueCount > 0 || timelineInfo.signalSemaphoreValueCount > 0)
  {
    // Only add timeline info if we have a timeline semaphore or validation layer complains.
    submitInfo.pNext = &timelineInfo;

    W_ASSERT_DEBUG(timelineInfo.waitSemaphoreValueCount == 0 || waitSemaphores.GetCount() == waitSemaphoreValues.GetCount(), "If a timeline semaphore is present, all semaphores need a wait value.");
    W_ASSERT_DEBUG(timelineInfo.signalSemaphoreValueCount == 0 || signalSemaphores.GetCount() == signalSemaphoreValues.GetCount(), "If a timeline semaphore is present, all semaphores need a signal value.");
  }
  W_ASSERT_DEBUG(waitSemaphores.GetCount() == waitStages.GetCount(), "Each wait semaphore needs a wait stage");

  submitInfo.waitSemaphoreCount = waitSemaphores.GetCount();
  submitInfo.pWaitSemaphores = waitSemaphores.GetData();
  submitInfo.pWaitDstStageMask = waitStages.GetData();
  submitInfo.signalSemaphoreCount = signalSemaphores.GetCount();
  submitInfo.pSignalSemaphores = signalSemaphores.GetData();

  {
    m_PerFrameData[m_uiCurrentPerFrameData].m_CommandBufferFences.PushBack(renderFence);
    VK_LOG_ERROR(m_GraphicsQueue.m_queue.submit(1, &submitInfo, renderFence));
  }

  m_pCommandEncoderImpl->AfterCommandBufferSubmit(renderFence);

  auto res = renderFence;
  ReclaimLater(renderFence);
  if (m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    ReclaimLater(m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer, m_pCommandBufferPool.Borrow());
  }
  return res;
}

WGALCommandEncoder* WGALDeviceVulkan::BeginCommandsPlatform(const char* szName)
{
  GetCurrentCommandBuffer();
#if W_ENABLED(W_USE_PROFILING)
  m_pPassTimingScope = WProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), szName);
#endif
  m_pCommandEncoderImpl->Reset();
  m_pCommandEncoder->InvalidateState();
  return m_pCommandEncoder.Borrow();
}

void WGALDeviceVulkan::EndCommandsPlatform(WGALCommandEncoder* pPass)
{
#if W_ENABLED(W_USE_PROFILING)
  WProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}


// State creation functions

WGALBlendState* WGALDeviceVulkan::CreateBlendStatePlatform(const WGALBlendStateCreationDescription& Description)
{
  WGALBlendStateVulkan* pState = W_NEW(&m_Allocator, WGALBlendStateVulkan, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    W_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyBlendStatePlatform(WGALBlendState* pBlendState)
{
  WGALBlendStateVulkan* pState = static_cast<WGALBlendStateVulkan*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pState);
}

WGALDepthStencilState* WGALDeviceVulkan::CreateDepthStencilStatePlatform(const WGALDepthStencilStateCreationDescription& Description)
{
  WGALDepthStencilStateVulkan* pVulkanDepthStencilState = W_NEW(&m_Allocator, WGALDepthStencilStateVulkan, Description);

  if (pVulkanDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pVulkanDepthStencilState;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanDepthStencilState);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyDepthStencilStatePlatform(WGALDepthStencilState* pDepthStencilState)
{
  WGALDepthStencilStateVulkan* pVulkanDepthStencilState = static_cast<WGALDepthStencilStateVulkan*>(pDepthStencilState);
  pVulkanDepthStencilState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanDepthStencilState);
}

WGALRasterizerState* WGALDeviceVulkan::CreateRasterizerStatePlatform(const WGALRasterizerStateCreationDescription& Description)
{
  WGALRasterizerStateVulkan* pVulkanRasterizerState = W_NEW(&m_Allocator, WGALRasterizerStateVulkan, Description);

  if (pVulkanRasterizerState->InitPlatform(this).Succeeded())
  {
    return pVulkanRasterizerState;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanRasterizerState);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyRasterizerStatePlatform(WGALRasterizerState* pRasterizerState)
{
  WGALRasterizerStateVulkan* pVulkanRasterizerState = static_cast<WGALRasterizerStateVulkan*>(pRasterizerState);
  pVulkanRasterizerState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanRasterizerState);
}

WGALSamplerState* WGALDeviceVulkan::CreateSamplerStatePlatform(const WGALSamplerStateCreationDescription& Description)
{
  WGALSamplerStateVulkan* pVulkanSamplerState = W_NEW(&m_Allocator, WGALSamplerStateVulkan, Description);

  if (pVulkanSamplerState->InitPlatform(this).Succeeded())
  {
    return pVulkanSamplerState;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanSamplerState);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroySamplerStatePlatform(WGALSamplerState* pSamplerState)
{
  WGALSamplerStateVulkan* pVulkanSamplerState = static_cast<WGALSamplerStateVulkan*>(pSamplerState);
  pVulkanSamplerState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanSamplerState);
}

void WGALDeviceVulkan::RecreateSamplerStatePlatform(WGALSamplerState* pSamplerState)
{
  WGALSamplerStateVulkan* pVulkanSamplerState = static_cast<WGALSamplerStateVulkan*>(pSamplerState);
  pVulkanSamplerState->DeInitPlatform(this).AssertSuccess();
  pVulkanSamplerState->InitPlatform(this).AssertSuccess();
}

WGALBindGroupLayout* WGALDeviceVulkan::CreateBindGroupLayoutPlatform(const WGALBindGroupLayoutCreationDescription& Description)
{
  WGALBindGroupLayoutVulkan* pVulkanBindGroupLayout = W_NEW(&m_Allocator, WGALBindGroupLayoutVulkan, Description);

  if (pVulkanBindGroupLayout->InitPlatform(this).Succeeded())
  {
    return pVulkanBindGroupLayout;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanBindGroupLayout);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyBindGroupLayoutPlatform(WGALBindGroupLayout* pBindGroupLayout)
{
  WGALBindGroupLayoutVulkan* pVulkanBindGroupLayout = static_cast<WGALBindGroupLayoutVulkan*>(pBindGroupLayout);
  pVulkanBindGroupLayout->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanBindGroupLayout);
}

WGALBindGroup* WGALDeviceVulkan::CreateBindGroupPlatform(const WGALBindGroupCreationDescription& Description)
{
  WGALBindGroupVulkan* pVulkanBindGroup = W_NEW(&m_Allocator, WGALBindGroupVulkan, Description);

  if (pVulkanBindGroup->InitPlatform(this).Succeeded())
  {
    return pVulkanBindGroup;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanBindGroup);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyBindGroupPlatform(WGALBindGroup* pBindGroup)
{
  WGALBindGroupVulkan* pVulkanBindGroup = static_cast<WGALBindGroupVulkan*>(pBindGroup);
  pVulkanBindGroup->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanBindGroup);
}

void WGALDeviceVulkan::RecreateBindGroupPlatform(WGALBindGroup* pBindGroup)
{
  WGALBindGroupVulkan* pVulkanBindGroup = static_cast<WGALBindGroupVulkan*>(pBindGroup);
  pVulkanBindGroup->DeInitPlatform(this).AssertSuccess();
  pVulkanBindGroup->InitPlatform(this).AssertSuccess();
}

WGALPipelineLayout* WGALDeviceVulkan::CreatePipelineLayoutPlatform(const WGALPipelineLayoutCreationDescription& Description)
{
  WGALPipelineLayoutVulkan* pVulkanPipelineLayout = W_NEW(&m_Allocator, WGALPipelineLayoutVulkan, Description);

  if (pVulkanPipelineLayout->InitPlatform(this).Succeeded())
  {
    return pVulkanPipelineLayout;
  }
  else
  {
    W_DELETE(&m_Allocator, pVulkanPipelineLayout);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyPipelineLayoutPlatform(WGALPipelineLayout* pPipelineLayout)
{
  WGALPipelineLayoutVulkan* pVulkanPipelineLayout = static_cast<WGALPipelineLayoutVulkan*>(pPipelineLayout);
  pVulkanPipelineLayout->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanPipelineLayout);
}

// Resource creation functions

WGALShader* WGALDeviceVulkan::CreateShaderPlatform(const WGALShaderCreationDescription& Description)
{
  WGALShaderVulkan* pShader = W_NEW(&m_Allocator, WGALShaderVulkan, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void WGALDeviceVulkan::DestroyShaderPlatform(WGALShader* pShader)
{
  WGALShaderVulkan* pVulkanShader = static_cast<WGALShaderVulkan*>(pShader);
  pVulkanShader->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanShader);
}

WGALBuffer* WGALDeviceVulkan::CreateBufferPlatform(
  const WGALBufferCreationDescription& Description, WArrayPtr<const WUInt8> pInitialData)
{
  WGALBufferVulkan* pBuffer = W_NEW(&m_Allocator, WGALBufferVulkan, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void WGALDeviceVulkan::DestroyBufferPlatform(WGALBuffer* pBuffer)
{
  WGALBufferVulkan* pVulkanBuffer = static_cast<WGALBufferVulkan*>(pBuffer);
  for (WUInt32 i = 0; i < m_PendingBufferCopies.GetCount();)
  {
    if (m_PendingBufferCopies[i].m_pDstBuffer == pVulkanBuffer)
    {
      m_PendingBufferCopies.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }
  pVulkanBuffer->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanBuffer);
}

WGALTexture* WGALDeviceVulkan::CreateTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  WGALTextureVulkan* pTexture = W_NEW(&m_Allocator, WGALTextureVulkan, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void WGALDeviceVulkan::DestroyTexturePlatform(WGALTexture* pTexture)
{
  WGALTextureVulkan* pVulkanTexture = static_cast<WGALTextureVulkan*>(pTexture);
  for (WInt32 i = (WInt32)m_PendingTextureCopies.GetCount() - 1; i >= 0; --i)
  {
    if (m_PendingTextureCopies[i].m_pDstTexture == pTexture)
      m_PendingTextureCopies.RemoveAtAndSwap(i);
  }

  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanTexture);
}

WGALTexture* WGALDeviceVulkan::CreateSharedTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle handle)
{
  WGALSharedTextureVulkan* pTexture = W_NEW(&m_Allocator, WGALSharedTextureVulkan, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void WGALDeviceVulkan::DestroySharedTexturePlatform(WGALTexture* pTexture)
{
  WGALSharedTextureVulkan* pVulkanTexture = static_cast<WGALSharedTextureVulkan*>(pTexture);
  for (WInt32 i = (WInt32)m_PendingTextureCopies.GetCount() - 1; i >= 0; --i)
  {
    if (m_PendingTextureCopies[i].m_pDstTexture == pTexture)
      m_PendingTextureCopies.RemoveAtAndSwap(i);
  }
  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanTexture);
}

WGALReadbackBuffer* WGALDeviceVulkan::CreateReadbackBufferPlatform(const WGALBufferCreationDescription& Description)
{
  WGALReadbackBufferVulkan* pReadbackBuffer = W_NEW(&m_Allocator, WGALReadbackBufferVulkan, Description);

  if (!pReadbackBuffer->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pReadbackBuffer);
    return nullptr;
  }

  return pReadbackBuffer;
}

void WGALDeviceVulkan::DestroyReadbackBufferPlatform(WGALReadbackBuffer* pReadbackBuffer)
{
  WGALReadbackBufferVulkan* pVulkanReadbackBuffer = static_cast<WGALReadbackBufferVulkan*>(pReadbackBuffer);

  pVulkanReadbackBuffer->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanReadbackBuffer);
}

WGALReadbackTexture* WGALDeviceVulkan::CreateReadbackTexturePlatform(const WGALTextureCreationDescription& Description)
{
  WGALReadbackTextureVulkan* pReadbackTexture = W_NEW(&m_Allocator, WGALReadbackTextureVulkan, Description);

  if (!pReadbackTexture->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pReadbackTexture);
    return nullptr;
  }

  return pReadbackTexture;
}

void WGALDeviceVulkan::DestroyReadbackTexturePlatform(WGALReadbackTexture* pReadbackTexture)
{
  WGALReadbackTextureVulkan* pVulkanReadbackTexture = static_cast<WGALReadbackTextureVulkan*>(pReadbackTexture);

  pVulkanReadbackTexture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanReadbackTexture);
}

WGALRenderTargetView* WGALDeviceVulkan::CreateRenderTargetViewPlatform(
  WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description)
{
  WGALRenderTargetViewVulkan* pRTView = W_NEW(&m_Allocator, WGALRenderTargetViewVulkan, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void WGALDeviceVulkan::DestroyRenderTargetViewPlatform(WGALRenderTargetView* pRenderTargetView)
{
  WGALRenderTargetViewVulkan* pVulkanRenderTargetView = static_cast<WGALRenderTargetViewVulkan*>(pRenderTargetView);
  pVulkanRenderTargetView->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVulkanRenderTargetView);
}

// Other rendering creation functions
WGALVertexDeclaration* WGALDeviceVulkan::CreateVertexDeclarationPlatform(const WGALVertexDeclarationCreationDescription& Description)
{
  WGALVertexDeclarationVulkan* pVertexDeclaration = W_NEW(&m_Allocator, WGALVertexDeclarationVulkan, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    W_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyVertexDeclarationPlatform(WGALVertexDeclaration* pVertexDeclaration)
{
  WGALVertexDeclarationVulkan* pVertexDeclarationVulkan = static_cast<WGALVertexDeclarationVulkan*>(pVertexDeclaration);
  pVertexDeclarationVulkan->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVertexDeclarationVulkan);
}

void WGALDeviceVulkan::UpdateBufferForNextFramePlatform(const WGALBuffer* pBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset)
{
  const WGALBufferVulkan* pBufferVulkan = static_cast<const WGALBufferVulkan*>(pBuffer);

  WPendingBufferCopyVulkan& copy = m_PendingBufferCopies.ExpandAndGetRef();
  // The staging pool is a frame allocator, but we explicitly allocate a buffer here that us used at the start of the next frame. To prevent race conditions, there is a delay of one frame inside WStagingBufferPoolVulkan::AfterBeginFrame to accomodate this fact.
  copy.m_SrcBuffer = GetStagingBufferPool().AllocateBuffer(sourceData.GetCount());
  WMemoryUtils::Copy(copy.m_SrcBuffer.m_Data.GetPtr(), sourceData.GetPtr(), sourceData.GetCount());

  copy.m_pDstBuffer = pBufferVulkan;
  copy.m_Region.srcOffset = copy.m_SrcBuffer.m_uiOffset;
  copy.m_Region.dstOffset = uiDestOffset;
  copy.m_Region.size = sourceData.GetCount();
}

void WGALDeviceVulkan::UpdateTextureForNextFramePlatform(const WGALTexture* pTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox)
{
  const WGALTextureVulkan* pTextureVulkan = static_cast<const WGALTextureVulkan*>(pTexture);

  const WVec3U32 boxExtents = destinationBox.GetExtents();
  const vk::Offset3D imageOffset = {(WInt32)destinationBox.m_vMin.x, (WInt32)destinationBox.m_vMin.y, (WInt32)destinationBox.m_vMin.z};
  const vk::Extent3D imageExtent = {boxExtents.x, boxExtents.y, boxExtents.z};

  vk::ImageSubresourceLayers subresourceLayers;
  subresourceLayers.aspectMask = WConversionUtilsVulkan::IsDepthFormat(pTextureVulkan->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  subresourceLayers.mipLevel = destinationSubResource.m_uiMipLevel;
  subresourceLayers.baseArrayLayer = destinationSubResource.m_uiArraySlice;
  subresourceLayers.layerCount = 1;

  const vk::Format format = pTextureVulkan->GetImageFormat();
  const TextureStagingLayout layout = ComputeTextureStagingLayout(format, imageExtent, sourceData);

  WPendingTextureCopyVulkan& copy = m_PendingTextureCopies.ExpandAndGetRef();
  // The staging pool is a frame allocator, but we explicitly allocate a buffer here that us used at the start of the next frame. To prevent race conditions, there is a delay of one frame inside WStagingBufferPoolVulkan::AfterBeginFrame to accomodate this fact.
  copy.m_SrcBuffer = m_pStagingBufferPool->AllocateBuffer(layout.m_uiUploadSize);

  WMemoryUtils::Copy(copy.m_SrcBuffer.m_Data.GetPtr(), sourceData.m_pData.GetPtr(), layout.m_uiUploadSize);

  copy.m_pDstTexture = pTextureVulkan;
  copy.m_Region.imageSubresource = subresourceLayers;
  copy.m_Region.imageOffset = imageOffset;
  copy.m_Region.imageExtent = imageExtent;
  copy.m_Region.bufferOffset = copy.m_SrcBuffer.m_uiOffset;
  copy.m_Region.bufferRowLength = layout.m_uiBufferRowLength;
  copy.m_Region.bufferImageHeight = layout.m_uiBufferImageHeight;
  copy.m_uiTotalSize = layout.m_uiUploadSize;
}

WEnum<WGALAsyncResult> WGALDeviceVulkan::GetTimestampResultPlatform(WGALTimestampHandle hTimestamp, WTime& result)
{
  return m_pQueryPool->GetTimestampResult(hTimestamp, result);
}

WEnum<WGALAsyncResult> WGALDeviceVulkan::GetOcclusionResultPlatform(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult)
{
  return m_pQueryPool->GetOcclusionQueryResult(hOcclusion, out_uiResult);
}

WEnum<WGALAsyncResult> WGALDeviceVulkan::GetFenceResultPlatform(WGALFenceHandle hFence, WTime timeout)
{
  if (m_pFenceQueue->GetCurrentFenceHandle() == hFence && timeout.IsPositive())
  {
    // Fence has not been submitted yet, force submit of the command buffer or we would deadlock here.
    Flush();
  }

  return m_pFenceQueue->GetFenceResult(hFence, timeout);
}

WResult WGALDeviceVulkan::LockBufferPlatform(const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_Memory) const
{
  const WGALReadbackBufferVulkan* pBufferVulkan = static_cast<const WGALReadbackBufferVulkan*>(pBuffer);

  void* pData = nullptr;
  VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::MapMemory(pBufferVulkan->GetAllocation(), &pData));
  WMemoryAllocatorVulkan::InvalidateAllocation(pBufferVulkan->GetAllocation());
  out_Memory = WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(pData), pBuffer->GetDescription().m_uiTotalSize);
  return W_SUCCESS;
}

void WGALDeviceVulkan::UnlockBufferPlatform(const WGALReadbackBuffer* pBuffer) const
{
  const WGALReadbackBufferVulkan* pBufferVulkan = static_cast<const WGALReadbackBufferVulkan*>(pBuffer);
  WMemoryAllocatorVulkan::UnmapMemory(pBufferVulkan->GetAllocation());
}

WResult WGALDeviceVulkan::LockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_Memory) const
{
  out_Memory.Clear();
  // #TODO_VULKAN readback fence
  auto pVulkanTexture = static_cast<const WGALReadbackTextureVulkan*>(pTexture->GetParentResource());
  const WGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();

  const vk::Format stagingFormat = GetFormatLookupTable().GetFormatInfo(pVulkanTexture->GetDescription().m_Format).m_readback;

  WHybridArray<WGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
  const WUInt32 uiBufferSize = WGALTextureVulkan::ComputeSubResourceOffsets(this, pVulkanTexture->GetDescription(), subResourceOffsets);

  const WUInt32 uiSubResources = subResources.GetCount();

  void* pData = nullptr;
  VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::MapMemory(pVulkanTexture->GetBufferAllocation(), &pData));
  WMemoryAllocatorVulkan::InvalidateAllocation(pVulkanTexture->GetBufferAllocation());
  const WUInt32 uiMipLevels = textureDesc.m_uiMipLevelCount;
  for (WUInt32 i = 0; i < uiSubResources; i++)
  {
    const WGALTextureSubresource& subRes = subResources[i];
    const WUInt32 uiSubresourceIndex = subRes.m_uiMipLevel + subRes.m_uiArraySlice * uiMipLevels;
    const WGALTextureVulkan::SubResourceOffset offset = subResourceOffsets[uiSubresourceIndex];
    WGALSystemMemoryDescription& memDesc = out_Memory.ExpandAndGetRef();
    const vk::Extent3D imageExtent = WGALTextureVulkan::GetMipLevelSize(textureDesc, subRes.m_uiMipLevel);
    const auto blockExtent = vk::blockExtent(stagingFormat);
    const WUInt8 uiBlockSize = vk::blockSize(stagingFormat);
    const WUInt32 uiBlockDepth = (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2];

    const WUInt32 uiRowPitch = (offset.m_uiRowLength / blockExtent[0]) * uiBlockSize;

    WUInt8* pSubResourceData = reinterpret_cast<WUInt8*>(pData) + offset.m_uiOffset;

    memDesc.m_pData = WMakeByteBlobPtr(pSubResourceData, offset.m_uiSize);
    memDesc.m_uiRowPitch = uiRowPitch;
    memDesc.m_uiSlicePitch = uiRowPitch * (offset.m_uiImageHeight / blockExtent[1]);
    W_ASSERT_DEBUG(memDesc.m_uiSlicePitch * uiBlockDepth == offset.m_uiSize, "");
  }

  return W_SUCCESS;
}

void WGALDeviceVulkan::UnlockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources) const
{
  auto pVulkanTexture = static_cast<const WGALReadbackTextureVulkan*>(pTexture->GetParentResource());

  WMemoryAllocatorVulkan::UnmapMemory(pVulkanTexture->GetBufferAllocation());
}

// Misc functions

void WGALDeviceVulkan::BeginFramePlatform(WArrayPtr<WGALSwapChain*> swapchains, const WUInt64 uiAppFrame)
{
  auto& pCommandEncoder = m_pCommandEncoderImpl;

  {
    W_PROFILE_SCOPE("CheckFences");
    // check if fence is reached
    for (WUInt64 uiFrame = m_uiSafeFrame + 1; uiFrame < m_uiFrameCounter; uiFrame++)
    {
      auto& perFrameData = m_PerFrameData[uiFrame % FRAMES];

      // if we accumulate more frames than we can hold in the ring buffer, force waiting for fences.
      const bool bForce = uiFrame % FRAMES == m_uiFrameCounter % FRAMES;

      if (perFrameData.m_uiFrame != ((WUInt64)-1))
      {
        W_ASSERT_DEBUG(uiFrame == perFrameData.m_uiFrame, "Frame data was likely overwritten and no longer matches the expected previous frame index. This should have been prevented by bForce above.");
        bool bFencesReached = true;
        for (vk::Fence fence : perFrameData.m_CommandBufferFences)
        {
          vk::Result fenceStatus = m_Device.getFenceStatus(fence);
          if (fenceStatus == vk::Result::eNotReady)
          {
            if (bForce)
              VK_ASSERT_DEV(m_Device.waitForFences(1, &fence, true, WMath::MaxValue<WUInt64>()));
            else
            {
              bFencesReached = false;
              break;
            }
          }
        }

        if (bFencesReached)
        {
          W_PROFILE_SCOPE("FrameCleanup");
          perFrameData.m_CommandBufferFences.Clear();
          // Not pretty, but as the fences are already in the deletion queue, we need to flush then from the fence queue before we call ReclaimResources below.
          m_pFenceQueue->FlushReadyFences();

          {
            W_LOCK(perFrameData.m_pendingDeletionsMutex);
            DeletePendingResources(perFrameData.m_pendingDeletionsPrevious);
          }
          {
            W_LOCK(perFrameData.m_reclaimResourcesMutex);
            ReclaimResources(perFrameData.m_reclaimResourcesPrevious);
          }
          m_uiSafeFrame = uiFrame;
        }
        else
          break;
      }
    }
  }

  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;

  // Must run after ReclaimResources above, which is what hands the descriptor sets of destroyed bind groups back to their pools.
  WDescriptorSetPoolVulkan::BeginFrame();

  m_pStagingBufferPool->AfterBeginFrame();
  m_pInitContext->AfterBeginFrame();

  {
    W_PROFILE_SCOPE("QueryPool");
    m_pQueryPool->AfterBeginFrame(GetCurrentCommandBuffer());
  }
  GetCurrentCommandBuffer();

#if W_ENABLED(W_USE_PROFILING)
  WStringBuilder sb;
  sb.SetFormat("RENDER FRAME {}", uiAppFrame);
  m_pFrameTimingScope = WProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), sb);
#endif

  Submit(false, true);

  W_PROFILE_SCOPE("AcquireNextRenderTargets");
  for (WGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void WGALDeviceVulkan::EndFramePlatform(WArrayPtr<WGALSwapChain*> swapchains)
{
  for (WGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if W_ENABLED(W_USE_PROFILING)
  {
    // In rare cases it could be that we submitted the command buffer already and don't have a new one allocated yet.
    GetCurrentCommandBuffer();
    WProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pFrameTimingScope);
  }
#endif

  // We need to prevent the init context from starting any uploads between submit and the update of the current frame counter. Otherwise we can't use the device's safe frame counter as uploads could slip in for frame N which has already been submitted.
  W_LOCK(m_pInitContext->AccessLock());

  if (m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    Submit();
  }
  m_pCommandEncoderImpl->EndFrame();

  {
    // Resources can be added to deletion / reclaim outside of the render frame. These will not be covered by the fences. To handle this, we swap the resources arrays so for any newly added resources we know they are not part of the batch that is deleted / reclaimed with the frame.
    auto& currentFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    {
      W_LOCK(currentFrameData.m_pendingDeletionsMutex);
      currentFrameData.m_pendingDeletionsPrevious.Swap(currentFrameData.m_pendingDeletions);
    }
    {
      W_LOCK(currentFrameData.m_reclaimResourcesMutex);
      currentFrameData.m_reclaimResourcesPrevious.Swap(currentFrameData.m_reclaimResources);
    }

    W_ASSERT_DEBUG(currentFrameData.m_CommandBufferFences.GetCount() > 0, "Each frame must have at least one fence to guard pending resource deletions");
  }
  m_uiFrameCounter.Increment();
  m_uiCurrentPerFrameData = (m_uiFrameCounter) % FRAMES;

  {
    WVulkanMemoryStatistics stats = WMemoryAllocatorVulkan::GetStats();
    WStats::SetStat("Vulkan/BlockCount", stats.m_uiBlockCount);
    WStats::SetStat("Vulkan/AllocationCount", stats.m_uiAllocationCount);
    WStats::SetStat("Vulkan/BlockBytes", stats.m_uiBlockBytes);
    WStats::SetStat("Vulkan/AllocationBytes", stats.m_uiAllocationBytes);

    WGALCommandEncoderImplVulkan::Statistics encoderStats = m_pCommandEncoderImpl->GetAndResetStatistics();
    WStats::SetStat("Vulkan/DescriptorSetsCreated", encoderStats.m_uiDescriptorSetsCreated);
    WStats::SetStat("Vulkan/DescriptorSetsUpdated", encoderStats.m_uiDescriptorSetsUpdated);
    WStats::SetStat("Vulkan/DescriptorSetsReused", encoderStats.m_uiDescriptorSetsReused);
    WStats::SetStat("Vulkan/DescriptorWrites", encoderStats.m_uiDescriptorWrites);
    WStats::SetStat("Vulkan/DynamicUniformBufferChanged", encoderStats.m_uiDynamicUniformBufferChanged);
    WStats::SetStat("Vulkan/DescriptorSetPools", WDescriptorSetPoolVulkan::GetPoolCount());
  }
}

WUInt64 WGALDeviceVulkan::GetCurrentFramePlatform() const
{
  return m_uiFrameCounter;
}
WUInt64 WGALDeviceVulkan::GetSafeFramePlatform() const
{
  return m_uiSafeFrame;
}

void WGALDeviceVulkan::FillCapabilitiesPlatform()
{
  vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice.getMemoryProperties();
  vk::PhysicalDeviceFeatures2 features = GetPhysicalDeviceFeatures();

  WUInt64 dedicatedMemory = 0;
  WUInt64 systemMemory = 0;
  for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
  {
    if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
    {
      dedicatedMemory += memProperties.memoryHeaps[i].size;
    }
    else
    {
      systemMemory += memProperties.memoryHeaps[i].size;
    }
  }

  {
    m_Capabilities.m_sAdapterName = WStringUtf8(m_Properties.properties.deviceName).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<WUInt64>(dedicatedMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<WUInt64>(systemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<WUInt64>(0); // TODO
    m_Capabilities.m_bHardwareAccelerated = m_Properties.properties.deviceType != vk::PhysicalDeviceType::eCpu;
    m_Capabilities.m_bSupportsTexelBuffer = true;
    m_Capabilities.m_bSupportsMultiSampledArrays = true;
  }

  m_Capabilities.m_bSupportsMultithreadedResourceCreation = true;
  m_Capabilities.m_bSupportsMultipleBindGroups = true;
  m_Capabilities.m_materialBufferLayout = WGALBufferLayout::Vulkan_Std430_relaxed;

  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::VertexShader] = true;
  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::HullShader] = features.features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::DomainShader] = features.features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::GeometryShader] = features.features.geometryShader;
  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::PixelShader] = true;
  m_Capabilities.m_bShaderStageSupported[WGALShaderStage::ComputeShader] = true; // we check this when creating the queue, always has to be supported
  m_Capabilities.m_bSupportsIndirectDraw = true;
  m_Capabilities.m_uiMaxPushConstantsSize = WMath::Min(m_Properties.properties.limits.maxPushConstantsSize, (WUInt32)WMath::MaxValue<WUInt16>());
  ;
#if W_ENABLED(W_PLATFORM_LINUX) || W_ENABLED(W_PLATFORM_ANDROID)
  m_Capabilities.m_bSupportsSharedTextures = m_Extensions.m_bTimelineSemaphore && m_Extensions.m_bExternalMemoryFd && m_Extensions.m_bExternalSemaphoreFd;
#elif W_ENABLED(W_PLATFORM_WINDOWS)
  m_Capabilities.m_bSupportsSharedTextures = m_Extensions.m_bTimelineSemaphore && m_Extensions.m_bExternalMemoryWin32 && m_Extensions.m_bExternalSemaphoreWin32;
#else
  W_ASSERT_NOT_IMPLEMENTED;
#endif
  m_Capabilities.m_bSupportsVSRenderTargetArrayIndex = m_Extensions.m_bShaderViewportIndexLayer;

  m_Capabilities.m_bSupportsConservativeRasterization = m_Extensions.m_bConservativeRasterization;
  m_Capabilities.m_bSupportsDepthBiasClamp = features.features.depthBiasClamp;
  m_Capabilities.m_bSupportsWireframe = features.features.fillModeNonSolid;

  m_Capabilities.m_FormatSupport.SetCount(WGALResourceFormat::ENUM_COUNT);
  for (WUInt32 i = 0; i < WGALResourceFormat::ENUM_COUNT; i++)
  {
    WGALResourceFormat::Enum format = (WGALResourceFormat::Enum)i;
    const WGALFormatLookupEntryVulkan& entry = m_FormatLookupTable.GetFormatInfo(format);
    const vk::FormatProperties formatProps = GetVulkanPhysicalDevice().getFormatProperties(entry.m_format);

    if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage)
    {
      m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::Texture);
      vk::ImageFormatProperties props;
      vk::Result res = GetVulkanPhysicalDevice().getImageFormatProperties(entry.m_format, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &props);
      if (res == vk::Result::eSuccess)
      {
        if (props.sampleCounts & vk::SampleCountFlagBits::e2)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA2x);
        if (props.sampleCounts & vk::SampleCountFlagBits::e4)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA4x);
        if (props.sampleCounts & vk::SampleCountFlagBits::e8)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA8x);
      }
    }
    if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage)
      m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::TextureRW);
    if (formatProps.bufferFeatures & vk::FormatFeatureFlagBits::eVertexBuffer)
      m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::VertexAttribute);
    if (WGALResourceFormat::IsDepthFormat(format))
    {
      if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::RenderTarget);
    }
    else
    {
      if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
        m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::RenderTarget);
    }
  }
}

void WGALDeviceVulkan::FlushPlatform()
{
  m_pCommandEncoderImpl->FlushPlatform();
}

void WGALDeviceVulkan::WaitIdlePlatform()
{
  WaitIdleInternal(false);
}

void WGALDeviceVulkan::WaitIdleInternal(bool bAddUpdateForNextFrameCommands)
{
  // Make sure command buffers get flushed. Also, no need to add a wait semaphore if we flush anyway, all commands will be done.
  Submit(false, bAddUpdateForNextFrameCommands);
  m_Device.waitIdle();

  DestroyDeadObjects();

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    // First, we wait for all fences for all submit calls. This is necessary to make sure no resources of the frame are still in use by the GPU.
    auto& perFrameData = m_PerFrameData[i];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_Device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_Device.waitForFences(1, &fence, true, 1000000000);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();
  }

  for (WUInt32 i = 0; i < FRAMES; ++i)
  {
    // Not pretty, but as the fences are already in the deletion queue, we need to flush then from the fence queue before we call ReclaimResources below.
    m_pFenceQueue->FlushReadyFences();
    {
      W_LOCK(m_PerFrameData[i].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletionsPrevious);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletions);
    }
    {
      W_LOCK(m_PerFrameData[i].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[i].m_reclaimResourcesPrevious);
      ReclaimResources(m_PerFrameData[i].m_reclaimResources);
    }
  }
}

vk::PhysicalDeviceFeatures2 WGALDeviceVulkan::GetPhysicalDeviceFeatures(void* pNext) const
{
  vk::PhysicalDeviceFeatures2 features;
  features.pNext = pNext;
  m_PhysicalDevice.getFeatures2(&features);

  WStringBuilder sDeviceName = WStringUtf8(m_Properties.properties.deviceName).GetView();
  // Tessellation test crashes on Mali
  if (sDeviceName == "Mali-G610")
    features.features.tessellationShader = false;
  return features;
}

vk::PipelineStageFlags WGALDeviceVulkan::GetSupportedStages() const
{
  return m_SupportedStages;
}

vk::PipelineStageFlags WGALDeviceVulkan::GetUnsupportedStages() const
{
  return m_UnsupportedStages;
}

WInt32 WGALDeviceVulkan::GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const
{

  for (WUInt32 i = 0; i < m_MemoryProperties.memoryTypeCount; ++i)
  {
    const vk::MemoryType& type = m_MemoryProperties.memoryTypes[i];
    if (requirements.memoryTypeBits & (1 << i) && (type.propertyFlags & properties))
    {
      return i;
    }
  }

  return -1;
}

void WGALDeviceVulkan::DeleteLaterImpl(const PendingDeletion& deletion)
{
  W_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletions.PushBack(deletion);
}

void WGALDeviceVulkan::ReclaimLater(const ReclaimResource& reclaim)
{
  W_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResources.PushBack(reclaim);
}

void WGALDeviceVulkan::DeletePendingResources(WDeque<PendingDeletion>& pendingDeletions)
{
  for (PendingDeletion& deletion : pendingDeletions)
  {
    switch (deletion.m_type)
    {
      case vk::ObjectType::eUnknown:
        if (deletion.m_flags.IsSet(PendingDeletionFlags::IsFileDescriptor))
        {
#if W_ENABLED(W_PLATFORM_LINUX) || W_ENABLED(W_PLATFORM_ANDROID)
          int fileDescriptor = static_cast<int>(reinterpret_cast<size_t>(deletion.m_pObject));
          int res = close(fileDescriptor);
          if (res == -1)
          {
            WLog::Error("close() failed on file descriptor with errno: {}", WArgErrno(errno));
          }
#else
          W_ASSERT_NOT_IMPLEMENTED;
#endif
        }
        else
        {
          W_REPORT_FAILURE("Unknown pending deletion");
        }
        break;
      case vk::ObjectType::eImageView:
      {
        vk::ImageView imageView = reinterpret_cast<vk::ImageView&>(deletion.m_pObject);
        m_Device.destroyImageView(imageView);
      }
      break;
      case vk::ObjectType::eImage:
      {
        auto& image = reinterpret_cast<vk::Image&>(deletion.m_pObject);
        OnBeforeImageDestroyed.Broadcast(OnBeforeImageDestroyedData{image, *this});
        if (deletion.m_flags.IsSet(PendingDeletionFlags::UsesExternalMemory))
        {
          m_Device.destroyImage(image);
          auto& deviceMemory = reinterpret_cast<vk::DeviceMemory&>(deletion.m_pContext);
          m_Device.freeMemory(deviceMemory);
        }
        else
        {
          WMemoryAllocatorVulkan::DestroyImage(image, deletion.m_allocation);
        }
      }
      break;
      case vk::ObjectType::eBuffer:
        WMemoryAllocatorVulkan::DestroyBuffer(reinterpret_cast<vk::Buffer&>(deletion.m_pObject), deletion.m_allocation);
        break;
      case vk::ObjectType::eBufferView:
        m_Device.destroyBufferView(reinterpret_cast<vk::BufferView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eFramebuffer:
        m_Device.destroyFramebuffer(reinterpret_cast<vk::Framebuffer&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eRenderPass:
        m_Device.destroyRenderPass(reinterpret_cast<vk::RenderPass&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSampler:
        m_Device.destroySampler(reinterpret_cast<vk::Sampler&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSemaphore:
        m_Device.destroySemaphore(reinterpret_cast<vk::Semaphore&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSwapchainKHR:
        m_Device.destroySwapchainKHR(reinterpret_cast<vk::SwapchainKHR&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSurfaceKHR:
        m_Instance.destroySurfaceKHR(reinterpret_cast<vk::SurfaceKHR&>(deletion.m_pObject));
        if (WWindowBase* pWindow = reinterpret_cast<WWindowBase*>(deletion.m_pContext))
        {
          pWindow->RemoveReference();
        }
        break;
      case vk::ObjectType::eShaderModule:
        m_Device.destroyShaderModule(reinterpret_cast<vk::ShaderModule&>(deletion.m_pObject));
        break;
      case vk::ObjectType::ePipeline:
        m_Device.destroyPipeline(reinterpret_cast<vk::Pipeline&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eDescriptorSetLayout:
        m_Device.destroyDescriptorSetLayout(reinterpret_cast<vk::DescriptorSetLayout&>(deletion.m_pObject));
        break;
      case vk::ObjectType::ePipelineLayout:
        m_Device.destroyPipelineLayout(reinterpret_cast<vk::PipelineLayout&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eDescriptorPool:
        m_Device.destroyDescriptorPool(reinterpret_cast<vk::DescriptorPool&>(deletion.m_pObject));
        break;
      default:
        W_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  pendingDeletions.Clear();
}

void WGALDeviceVulkan::ReclaimResources(WDeque<ReclaimResource>& resources)
{
  for (ReclaimResource& resource : resources)
  {
    switch (resource.m_type)
    {
      case vk::ObjectType::eSemaphore:
        WSemaphorePoolVulkan::ReclaimSemaphore(reinterpret_cast<vk::Semaphore&>(resource.m_pObject));
        break;
      case vk::ObjectType::eFence:
        WFencePoolVulkan::ReclaimFence(reinterpret_cast<vk::Fence&>(resource.m_pObject));
        break;
      case vk::ObjectType::eCommandBuffer:
        static_cast<WCommandBufferPoolVulkan*>(resource.m_pContext)->ReclaimCommandBuffer(reinterpret_cast<vk::CommandBuffer&>(resource.m_pObject));
        break;
      case vk::ObjectType::eDescriptorPool:
        WTransientDescriptorSetPoolVulkan::ReclaimPool(reinterpret_cast<vk::DescriptorPool&>(resource.m_pObject));
        break;
      case vk::ObjectType::eDescriptorSet:
        static_cast<WDescriptorSetPoolVulkan*>(resource.m_pContext)->ReclaimDescriptorSet(reinterpret_cast<vk::DescriptorSet&>(resource.m_pObject), {resource.m_Data});
        break;
      default:
        W_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  resources.Clear();
}

void WGALDeviceVulkan::FillFormatLookupTable()
{
  /// The list below is in the same order as the WGALResourceFormat enum. No format should be missing except the ones that are just different names for the same enum value.
  vk::Format R32G32B32A32_Formats[] = {vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Uint, vk::Format::eR32G32B32A32Sint};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAFloat, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sfloat, R32G32B32A32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Uint, R32G32B32A32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sint, R32G32B32A32_Formats));

  vk::Format R32G32B32_Formats[] = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Uint, vk::Format::eR32G32B32Sint};
  // TODO 3-channel formats are not really supported under vulkan judging by experience
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBFloat, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sfloat, R32G32B32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBUInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Uint, R32G32B32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sint, R32G32B32_Formats));

  // TODO dunno if these are actually supported for the respective Vulkan device
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::B5G6R5UNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR5G6B5UnormPack16));

  vk::Format B8G8R8A8_Formats[] = {vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Srgb};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BGRAUByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Unorm, B8G8R8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BGRAUByteNormalizedsRGB, WGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Srgb, B8G8R8A8_Formats));

  vk::Format R16G16B16A16_Formats[] = {vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16A16Uint, vk::Format::eR16G16B16A16Unorm, vk::Format::eR16G16B16A16Sint, vk::Format::eR16G16B16A16Snorm};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAHalf, WGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sfloat, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUShort, WGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Uint, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Unorm, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAShort, WGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sint, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Snorm, R16G16B16A16_Formats));

  vk::Format R32G32_Formats[] = {vk::Format::eR32G32Sfloat, vk::Format::eR32G32Uint, vk::Format::eR32G32Sint};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGFloat, WGALFormatLookupEntryVulkan(vk::Format::eR32G32Sfloat, R32G32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32Uint, R32G32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGInt, WGALFormatLookupEntryVulkan(vk::Format::eR32G32Sint, R32G32_Formats));

  vk::Format R10G10B10A2_Formats[] = {vk::Format::eA2B10G10R10UintPack32, vk::Format::eA2B10G10R10UnormPack32};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGB10A2UInt, WGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UintPack32, R10G10B10A2_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGB10A2UIntNormalized, WGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UnormPack32, R10G10B10A2_Formats));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RG11B10Float, WGALFormatLookupEntryVulkan(vk::Format::eB10G11R11UfloatPack32));

  vk::Format R8G8B8A8_Formats[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Uint, vk::Format::eR8G8B8A8Snorm, vk::Format::eR8G8B8A8Sint};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Unorm, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByteNormalizedsRGB, WGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Srgb, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByte, WGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Uint, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Snorm, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAByte, WGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Sint, R8G8B8A8_Formats));

  vk::Format R16G16_Formats[] = {vk::Format::eR16G16Sfloat, vk::Format::eR16G16Uint, vk::Format::eR16G16Unorm, vk::Format::eR16G16Sint, vk::Format::eR16G16Snorm};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGHalf, WGALFormatLookupEntryVulkan(vk::Format::eR16G16Sfloat, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUShort, WGALFormatLookupEntryVulkan(vk::Format::eR16G16Uint, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16G16Unorm, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGShort, WGALFormatLookupEntryVulkan(vk::Format::eR16G16Sint, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16G16Snorm, R16G16_Formats));

  vk::Format R8G8_Formats[] = {vk::Format::eR8G8Uint, vk::Format::eR8G8Unorm, vk::Format::eR8G8Sint, vk::Format::eR8G8Snorm};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUByte, WGALFormatLookupEntryVulkan(vk::Format::eR8G8Uint, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8G8Unorm, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGByte, WGALFormatLookupEntryVulkan(vk::Format::eR8G8Sint, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8G8Snorm, R8G8_Formats));

  vk::Format R32_Formats[] = {vk::Format::eR32Sfloat, vk::Format::eR32Uint, vk::Format::eR32Sint};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RFloat, WGALFormatLookupEntryVulkan(vk::Format::eR32Sfloat, R32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUInt, WGALFormatLookupEntryVulkan(vk::Format::eR32Uint, R32_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RInt, WGALFormatLookupEntryVulkan(vk::Format::eR32Sint, R32_Formats));

  vk::Format R16_Formats[] = {vk::Format::eR16Sfloat, vk::Format::eR16Uint, vk::Format::eR16Unorm, vk::Format::eR16Sint, vk::Format::eR16Snorm};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RHalf, WGALFormatLookupEntryVulkan(vk::Format::eR16Sfloat, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUShort, WGALFormatLookupEntryVulkan(vk::Format::eR16Uint, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16Unorm, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RShort, WGALFormatLookupEntryVulkan(vk::Format::eR16Sint, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RShortNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR16Snorm, R16_Formats));

  vk::Format R8_Formats[] = {vk::Format::eR8Uint, vk::Format::eR8Unorm, vk::Format::eR8Sint, vk::Format::eR8Snorm};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUByte, WGALFormatLookupEntryVulkan(vk::Format::eR8Uint, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8Unorm, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RByte, WGALFormatLookupEntryVulkan(vk::Format::eR8Sint, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8Snorm, R8_Formats));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::AUByteNormalized, WGALFormatLookupEntryVulkan(vk::Format::eR8Unorm));

  auto SelectDepthFormat = [&](const std::vector<vk::Format>& list) -> vk::Format
  {
    for (auto& format : list)
    {
      vk::FormatProperties formatProperties;
      m_PhysicalDevice.getFormatProperties(format, &formatProperties);
      if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        return format;
    }
    return vk::Format::eUndefined;
  };

  auto SelectStorageFormat = [](vk::Format depthFormat) -> vk::Format
  {
    switch (depthFormat)
    {
      case vk::Format::eD16Unorm:
        return vk::Format::eR16Unorm;
      case vk::Format::eD16UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD24UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD32Sfloat:
        return vk::Format::eR32Sfloat;
      case vk::Format::eD32SfloatS8Uint:
        return vk::Format::eR32Sfloat;
      default:
        return vk::Format::eUndefined;
    }
  };

  // Select smallest available depth format.  #TODO_VULKAN support packed eX8D24UnormPack32?
  vk::Format depthFormat = SelectDepthFormat({vk::Format::eD16Unorm, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint});
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::D16, WGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  // Select closest depth stencil format.
  depthFormat = SelectDepthFormat({vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint});
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::D24S8, WGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  // Select biggest depth format.
  depthFormat = SelectDepthFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm});
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::DFloat, WGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  vk::Format BC1_Formats[] = {vk::Format::eBc1RgbaUnormBlock, vk::Format::eBc1RgbaSrgbBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC1, WGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaUnormBlock, BC1_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC1sRGB, WGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaSrgbBlock, BC1_Formats));

  vk::Format BC2_Formats[] = {vk::Format::eBc2UnormBlock, vk::Format::eBc2SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC2, WGALFormatLookupEntryVulkan(vk::Format::eBc2UnormBlock, BC2_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC2sRGB, WGALFormatLookupEntryVulkan(vk::Format::eBc2SrgbBlock, BC2_Formats));

  vk::Format BC3_Formats[] = {vk::Format::eBc3UnormBlock, vk::Format::eBc3SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC3, WGALFormatLookupEntryVulkan(vk::Format::eBc3UnormBlock, BC3_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC3sRGB, WGALFormatLookupEntryVulkan(vk::Format::eBc3SrgbBlock, BC3_Formats));

  vk::Format BC4_Formats[] = {vk::Format::eBc4UnormBlock, vk::Format::eBc4SnormBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC4UNormalized, WGALFormatLookupEntryVulkan(vk::Format::eBc4UnormBlock, BC4_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC4Normalized, WGALFormatLookupEntryVulkan(vk::Format::eBc4SnormBlock, BC4_Formats));

  vk::Format BC5_Formats[] = {vk::Format::eBc5UnormBlock, vk::Format::eBc5SnormBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC5UNormalized, WGALFormatLookupEntryVulkan(vk::Format::eBc5UnormBlock, BC5_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC5Normalized, WGALFormatLookupEntryVulkan(vk::Format::eBc5SnormBlock, BC5_Formats));

  vk::Format BC6_Formats[] = {vk::Format::eBc6HUfloatBlock, vk::Format::eBc6HSfloatBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC6UFloat, WGALFormatLookupEntryVulkan(vk::Format::eBc6HUfloatBlock, BC6_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC6Float, WGALFormatLookupEntryVulkan(vk::Format::eBc6HSfloatBlock, BC6_Formats));

  vk::Format BC7_Formats[] = {vk::Format::eBc7UnormBlock, vk::Format::eBc7SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC7UNormalized, WGALFormatLookupEntryVulkan(vk::Format::eBc7UnormBlock, BC7_Formats));
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC7UNormalizedsRGB, WGALFormatLookupEntryVulkan(vk::Format::eBc7SrgbBlock, BC7_Formats));

  if (false)
  {
    W_LOG_BLOCK("GAL Resource Formats");
    for (WUInt32 i = 1; i < WGALResourceFormat::ENUM_COUNT; i++)
    {
      const WGALFormatLookupEntryVulkan& entry = m_FormatLookupTable.GetFormatInfo((WGALResourceFormat::Enum)i);

      vk::FormatProperties formatProperties;
      m_PhysicalDevice.getFormatProperties(entry.m_format, &formatProperties);

      const bool bSampled = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
      const bool bColorAttachment = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment);
      const bool bDepthAttachment = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment);
      const bool bStorageImage = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage);

      const bool bTexel = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eUniformTexelBuffer);
      const bool bStorageTexel = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eStorageTexelBuffer);
      const bool bVertex = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eVertexBuffer);

      WStringBuilder sTemp;
      WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), i, sTemp, WReflectionUtils::EnumConversionMode::ValueNameOnly);

      WLog::Info("OptTiling S: {}, UAV: {}, CA: {}, DA: {}. Buffer: T: {}, ST: {}, V: {}, Format {} -> {}", bSampled ? 1 : 0, bStorageImage ? 1 : 0, bColorAttachment ? 1 : 0, bDepthAttachment ? 1 : 0, bTexel ? 1 : 0, bStorageTexel ? 1 : 0, bVertex ? 1 : 0, sTemp, vk::to_string(entry.m_format).c_str());
    }
  }
}

const WGALSharedTexture* WGALDeviceVulkan::GetSharedTexture(WGALTextureHandle hTexture) const
{
  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const WGALSharedTextureVulkan*>(pTexture->GetParentResource());
}

void WGALDeviceVulkan::AddWaitSemaphore(const SemaphoreInfo& waitSemaphore)
{
  // #TODO_VULKAN Assert is in render pipeline, thread safety
  if (waitSemaphore.m_type == vk::SemaphoreType::eTimeline)
    m_WaitSemaphores.InsertAt(0, waitSemaphore);
  else
    m_WaitSemaphores.PushBack(waitSemaphore);
}

void WGALDeviceVulkan::AddSignalSemaphore(const SemaphoreInfo& signalSemaphore)
{
  // #TODO_VULKAN Assert is in render pipeline, thread safety
  if (signalSemaphore.m_type == vk::SemaphoreType::eTimeline)
    m_SignalSemaphores.InsertAt(0, signalSemaphore);
  else
    m_SignalSemaphores.PushBack(signalSemaphore);
}

WGALGraphicsPipeline* WGALDeviceVulkan::CreateGraphicsPipelinePlatform(const WGALGraphicsPipelineCreationDescription& Description)
{
  WGALGraphicsPipelineVulkan* pGraphicsPipeline = W_NEW(&m_Allocator, WGALGraphicsPipelineVulkan, Description);

  if (pGraphicsPipeline->InitPlatform(this).Succeeded())
  {
    return pGraphicsPipeline;
  }
  else
  {
    W_DELETE(&m_Allocator, pGraphicsPipeline);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyGraphicsPipelinePlatform(WGALGraphicsPipeline* pGraphicsPipeline)
{
  WGALGraphicsPipelineVulkan* pGraphicsPipelineVulkan = static_cast<WGALGraphicsPipelineVulkan*>(pGraphicsPipeline);
  pGraphicsPipelineVulkan->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pGraphicsPipelineVulkan);
}

WGALComputePipeline* WGALDeviceVulkan::CreateComputePipelinePlatform(const WGALComputePipelineCreationDescription& Description)
{
  WGALComputePipelineVulkan* pComputePipeline = W_NEW(&m_Allocator, WGALComputePipelineVulkan, Description);

  if (pComputePipeline->InitPlatform(this).Succeeded())
  {
    return pComputePipeline;
  }
  else
  {
    W_DELETE(&m_Allocator, pComputePipeline);
    return nullptr;
  }
}

void WGALDeviceVulkan::DestroyComputePipelinePlatform(WGALComputePipeline* pComputePipeline)
{
  WGALComputePipelineVulkan* pComputePipelineVulkan = static_cast<WGALComputePipelineVulkan*>(pComputePipeline);
  pComputePipelineVulkan->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pComputePipelineVulkan);
}

WDescriptorWritePoolVulkan& WGALDeviceVulkan::GetDescriptorWritePool() const
{
  return m_pCommandEncoderImpl->GetDescriptorWritePool();
}

W_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_DeviceVulkan);

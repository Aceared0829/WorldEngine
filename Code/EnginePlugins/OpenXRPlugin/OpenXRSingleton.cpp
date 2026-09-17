#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>
#include <OpenXRPlugin/Graphics/OpenXRSwapChain.h>
#include <OpenXRPlugin/Input/OpenXRHandTracking.h>
#include <OpenXRPlugin/Input/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>
#include <OpenXRPlugin/Utils/OpenXRConversionUtils.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

static_assert(WGALMSAASampleCount::None == 1);
static_assert(WGALMSAASampleCount::TwoSamples == 2);
static_assert(WGALMSAASampleCount::FourSamples == 4);
static_assert(WGALMSAASampleCount::EightSamples == 8);

static WOpenXR g_OpenXRSingleton;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, OpenXR)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_OpenXRSingleton.OnEngineStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_OpenXRSingleton.OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

W_IMPLEMENT_SINGLETON(WOpenXR);

XrBool32 XRAPI_CALL xrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes, const XrDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  switch (messageSeverity)
  {
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      WLog::Debug("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      WLog::Info("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      WLog::Warning("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      WLog::Error("XR: {}", pCallbackData->message);
      break;
    default:
      break;
  }
  // Only layers are allowed to return true here.
  return XR_FALSE;
}

WOpenXR::WOpenXR()
  : m_SingletonRegistrar(this)
{
}

WOpenXR::~WOpenXR() = default;

bool WOpenXR::GetDepthComposition() const
{
  return m_Extensions.m_bDepthComposition;
}

bool WOpenXR::IsHmdPresent() const
{
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  uint64_t systemId = XR_NULL_SYSTEM_ID;
  XrResult res = xrGetSystem(m_pInstance, &systemInfo, &systemId);

  return res == XrResult::XR_SUCCESS;
}

XrResult WOpenXR::SelectExtensions(WHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  WUInt32 extensionCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
  WDynamicArray<XrExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.GetData()));

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> XrResult
  {
    for (const XrExtensionProperties& prop : extensionProperties)
    {
      if (WStringUtils::IsEqual(prop.extensionName, extensionName))
      {
        extensions.PushBack(extensionName);
        enableFlag = true;
        return XR_SUCCESS;
      }
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

  // Let the graphics binding add its required extension
  XrResult graphicsExtResult = m_pGraphicsBinding->SelectExtension(extensions, extensionProperties);
  if (graphicsExtResult != XR_SUCCESS)
  {
    m_pGraphicsBinding.Clear();
    return graphicsExtResult;
  }

  AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, m_Extensions.m_bDepthComposition);
  AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME, m_Extensions.m_bUnboundedReferenceSpace);
  AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME, m_Extensions.m_bSpatialAnchor);
  AddExtIfSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME, m_Extensions.m_bHandTracking);
  AddExtIfSupported(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME, m_Extensions.m_bHandInteraction);
  AddExtIfSupported(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME, m_Extensions.m_bHandTrackingMesh);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  AddExtIfSupported(XR_EXT_DEBUG_UTILS_EXTENSION_NAME, m_Extensions.m_bDebugUtils);
#endif

  return XR_SUCCESS;
}

XrResult WOpenXR::SelectLayers(WHybridArray<const char*, 6>& layers)
{
  WUInt32 layerCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
  WDynamicArray<XrApiLayerProperties> layerProperties;
  layerProperties.SetCount(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(layerCount, &layerCount, layerProperties.GetData()));

  // Add a specific layer to the list of layers to be enabled, if it is supported.
  auto AddLayerIfSupported = [&](const char* layerName, bool& enableFlag) -> XrResult
  {
    for (const XrApiLayerProperties& prop : layerProperties)
    {
      if (WStringUtils::IsEqual(prop.layerName, layerName))
      {
        layers.PushBack(layerName);
        enableFlag = true;
        return XR_SUCCESS;
      }
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  AddLayerIfSupported("XR_APILAYER_LUNARG_core_validation", m_Extensions.m_bValidation);
#endif

  return XR_SUCCESS;
}


#define W_GET_INSTANCE_PROC_ADDR(name) (void)xrGetInstanceProcAddr(m_pInstance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_Extensions.pfn_##name));

WResult WOpenXR::InitInstance(WGALDevice* pDevice)
{
  if (m_pInstance != XR_NULL_HANDLE)
    return W_SUCCESS;

  // Create the graphics binding based on the current renderer
  m_pGraphicsBinding = WOpenXRGraphicsBinding::Create(this, pDevice);
  if (!m_pGraphicsBinding)
  {
    WLog::Error("OpenXR: Failed to create graphics binding for current renderer");
    return W_SUCCESS;
  }

  // Build out the extensions to enable. Some extensions are required and some are optional.
  WHybridArray<const char*, 6> enabledExtensions;
  if (SelectExtensions(enabledExtensions) != XR_SUCCESS)
    return W_FAILURE;

  WHybridArray<const char*, 6> enabledLayers;
  if (SelectLayers(enabledLayers) != XR_SUCCESS)
    return W_FAILURE;

  // Create the instance with desired extensions.
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetCount();
  createInfo.enabledExtensionNames = enabledExtensions.GetData();
  createInfo.enabledApiLayerCount = (uint32_t)enabledLayers.GetCount();
  createInfo.enabledApiLayerNames = enabledLayers.GetData();

  WStringUtils::Copy(createInfo.applicationInfo.applicationName, W_ARRAY_SIZE(createInfo.applicationInfo.applicationName), WApplication::GetApplicationInstance()->GetApplicationName());
  WStringUtils::Copy(createInfo.applicationInfo.engineName, W_ARRAY_SIZE(createInfo.applicationInfo.engineName), "WorldEngine");
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
  createInfo.applicationInfo.applicationVersion = 1;
  XrResult res = xrCreateInstance(&createInfo, &m_pInstance);
  if (res != XR_SUCCESS)
  {
    WLog::Error("InitSystem xrCreateInstance failed: {}", res);
    DeinitInstance();
    return W_FAILURE;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  res = xrGetInstanceProperties(m_pInstance, &instanceProperties);
  if (res != XR_SUCCESS)
  {
    WLog::Error("InitSystem xrGetInstanceProperties failed: {}", res);
    DeinitInstance();
    return W_FAILURE;
  }

  WStringBuilder sTemp;
  m_Info.m_sDeviceDriver = WConversionUtils::ToString(instanceProperties.runtimeVersion, sTemp);

  m_pGraphicsBinding->LoadFunctionPointers(m_pInstance);

  if (m_Extensions.m_bSpatialAnchor)
  {
    W_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorMSFT);
    W_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorSpaceMSFT);
    W_GET_INSTANCE_PROC_ADDR(xrDestroySpatialAnchorMSFT);
  }

  if (m_Extensions.m_bHandTracking)
  {
    W_GET_INSTANCE_PROC_ADDR(xrCreateHandTrackerEXT);
    W_GET_INSTANCE_PROC_ADDR(xrDestroyHandTrackerEXT);
    W_GET_INSTANCE_PROC_ADDR(xrLocateHandJointsEXT);
  }

  if (m_Extensions.m_bHandTrackingMesh)
  {
    W_GET_INSTANCE_PROC_ADDR(xrCreateHandMeshSpaceMSFT);
    W_GET_INSTANCE_PROC_ADDR(xrUpdateHandMeshMSFT);
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (m_Extensions.m_bDebugUtils)
  {
    W_GET_INSTANCE_PROC_ADDR(xrCreateDebugUtilsMessengerEXT);
    W_GET_INSTANCE_PROC_ADDR(xrDestroyDebugUtilsMessengerEXT);
  }
#endif

  WLog::Success("OpenXR: {0} v{1} initialized successfully.", instanceProperties.runtimeName, instanceProperties.runtimeVersion);
  return W_SUCCESS;
}

void WOpenXR::DeinitInstance()
{
  m_pGraphicsBinding.Clear();

  if (m_pInstance)
  {
    xrDestroyInstance(m_pInstance);
    m_pInstance = XR_NULL_HANDLE;
  }
}

WResult WOpenXR::Initialize()
{
  if (!m_pInstance)
  {
    WLog::Error("OpenXR: Instance creation failed");
    return W_FAILURE;
  }
  if (!m_SystemId)
  {
    WLog::Error("OpenXR: system creation failed");
    return W_FAILURE;
  }
  m_pInput = W_DEFAULT_NEW(WOpenXRInputDevice, this);

  m_ExecutionEventsId = WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(WMakeDelegate(&WOpenXR::GameApplicationEventHandler, this));

  return W_SUCCESS;
}

void WOpenXR::Deinitialize()
{
  if (m_ExecutionEventsId != 0)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }

  WGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  DeinitSession();
  m_pInput = nullptr;
}

bool WOpenXR::IsInitialized() const
{
  return m_pInstance != XR_NULL_HANDLE;
}

const WHMDInfo& WOpenXR::GetHmdInfo() const
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  return m_Info;
}

WXRInputDevice& WOpenXR::GetXRInput() const
{
  return *(m_pInput.Borrow());
}

WRegisteredWndHandle WOpenXR::CreateXRWindow(WView* pView, WGALMSAASampleCount::Enum msaaCount, WUniquePtr<WWindowBase> pCompanionWindow, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutput)
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  XrResult res = InitSession();
  if (res != XrResult::XR_SUCCESS)
  {
    WLog::Error("InitSession failed: {}", res);
    return {};
  }

  WGALXRSwapChain::SetFactoryMethod([this, msaaCount](WXRInterface* pXrInterface) -> WGALSwapChainHandle
    { return WGALDevice::GetDefaultDevice()->CreateSwapChain([this, pXrInterface, msaaCount](WAllocator* pAllocator) -> WGALSwapChain*
        { return W_NEW(pAllocator, WGALOpenXRSwapChain, this, msaaCount); }); });
  W_SCOPE_EXIT(WGALXRSwapChain::SetFactoryMethod({}););

  m_hSwapChain = WGALXRSwapChain::Create(this);
  if (m_hSwapChain.IsInvalidated())
  {
    DeinitSession();
    WLog::Error("InitSwapChain failed: {}", res);
    return {};
  }

  const WGALOpenXRSwapChain* pSwapChain = static_cast<const WGALOpenXRSwapChain*>(WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  m_Info.m_vEyeRenderTargetSize = pSwapChain->GetRenderTargetSize();

  {
    W_ASSERT_DEV(pView->GetCamera() != nullptr, "The provided view requires a camera to be set.");
    SetHMDCamera(pView->GetCamera());
  }

  auto pWinMan = WWindowManager::GetSingleton();

  W_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");
  W_ASSERT_DEV(pCompanionWindow == nullptr || SupportsCompanionView(), "If a companionWindow is set, SupportsCompanionView() must be true.");

  WUniquePtr<WWindowXR> pWindowXR = W_DEFAULT_NEW(WWindowXR, this, std::move(pCompanionWindow));
  WUniquePtr<WWindowOutputTargetXR> pOutputTargetXR = W_DEFAULT_NEW(WWindowOutputTargetXR, this, std::move(pCompanionWindowOutput));

  m_pCompanion = static_cast<WWindowOutputTargetXR*>(pOutputTargetXR.Borrow());

  WRegisteredWndHandle windowId = pWinMan->Register("OpenXR", this, std::move(pWindowXR));
  pWinMan->SetOutputTarget(windowId, std::move(pOutputTargetXR));
  pWinMan->SetDestroyCallback(windowId, [this](WRegisteredWndHandle)
    { this->OnActorDestroyed(); });

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  m_hView = pView->GetHandle();

  pView->SetSwapChain(m_hSwapChain);

  pView->SetViewport(WRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return windowId;
}

void WOpenXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  SetHMDCamera(nullptr);

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();

  pDevice->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  pDevice->WaitIdle();

  DeinitSession();
}

bool WOpenXR::SupportsCompanionView()
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)
  return true;
#else
  // E.g. on UWP OpenXR creates its own main window and other resources that conflict with our window.
  // Thus we must prevent the creation of a companion view or OpenXR crashes.
  return false;
#endif
}

XrSpace WOpenXR::GetBaseSpace() const
{
  return m_StageSpace == WXRStageSpace::Standing ? m_pSceneSpace : m_pLocalSpace;
}

void WOpenXR::OnEngineStartup()
{
  m_GALdeviceEventsId = WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WOpenXR::GALDeviceEventHandler, this));
}

void WOpenXR::OnEngineShutdown()
{
  if (m_GALdeviceEventsId != 0)
  {
    WGALDevice::s_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }
}

XrResult WOpenXR::InitSystem()
{
  W_ASSERT_DEV(m_SystemId == XR_NULL_SYSTEM_ID, "OpenXR actor already exists.");
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystem(m_pInstance, &systemInfo, &m_SystemId), DeinitSystem);

  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_pInstance, m_SystemId, &systemProperties), DeinitSystem);
  m_Info.m_sDeviceName = systemProperties.systemName;

  return XrResult::XR_SUCCESS;
}

void WOpenXR::DeinitSystem()
{
  m_SystemId = XR_NULL_SYSTEM_ID;
}

XrResult WOpenXR::InitSession()
{
  W_ASSERT_DEV(m_pSession == XR_NULL_HANDLE, "");

  WUInt32 count;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, 0, &count, nullptr), DeinitSystem);

  WHybridArray<XrEnvironmentBlendMode, 4> environmentBlendModes;
  environmentBlendModes.SetCount(count);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, count, &count, environmentBlendModes.GetData()), DeinitSession);

  // Select preferred blend mode.
  m_BlendMode = environmentBlendModes[0];

  XR_SUCCEED_OR_CLEANUP_LOG(InitGraphicsPlugin(), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(InitDebugMessenger(), DeinitSession);

  XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionCreateInfo.systemId = m_SystemId;

  // Set the graphics binding from the abstraction
  if (!m_pGraphicsBinding)
  {
    WLog::Error("No graphics binding available for OpenXR session");
    return XrResult::XR_ERROR_GRAPHICS_DEVICE_INVALID;
  }
  sessionCreateInfo.next = m_pGraphicsBinding->GetGraphicsBinding();

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSession(m_pInstance, &sessionCreateInfo, &m_pSession), DeinitSession);

  XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  spaceCreateInfo.poseInReferenceSpace = WOpenXRConversionUtils::ConvertTransform(WTransform::MakeIdentity());
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pSceneSpace), DeinitSession);

  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pLocalSpace), DeinitSession);

  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->CreateActions(m_pSession, m_pSceneSpace), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->AttachSessionActionSets(m_pSession), DeinitSession);

  m_RenderWorldEventId = WRenderWorld::GetRenderEvent().AddEventHandler(WMakeDelegate(&WOpenXR::OnRenderWorldEvent, this));

  SetStageSpace(WXRStageSpace::Standing);
  if (m_Extensions.m_bSpatialAnchor)
  {
    m_pAnchors = W_DEFAULT_NEW(WOpenXRSpatialAnchors, this);
  }
  if (m_Extensions.m_bHandTracking && WOpenXRHandTracking::IsHandTrackingSupported(this))
  {
    m_pHandTracking = W_DEFAULT_NEW(WOpenXRHandTracking, this);
  }
  return XrResult::XR_SUCCESS;
}

void WOpenXR::DeinitSession()
{
  m_pCompanion = nullptr;
  m_bSessionRunning = false;
  m_bExitRenderLoop = false;
  m_bRequestRestart = false;
  m_bRenderInProgress = false;
  m_SessionState = XR_SESSION_STATE_UNKNOWN;

  m_pHandTracking = nullptr;
  m_pAnchors = nullptr;

  if (m_RenderWorldEventId != 0)
  {
    WRenderWorld::GetRenderEvent().RemoveEventHandler(m_RenderWorldEventId);
  }

  if (m_pSceneSpace)
  {
    xrDestroySpace(m_pSceneSpace);
    m_pSceneSpace = XR_NULL_HANDLE;
  }

  if (m_pLocalSpace)
  {
    xrDestroySpace(m_pLocalSpace);
    m_pLocalSpace = XR_NULL_HANDLE;
  }

  m_pInput->DestroyActions();

  if (m_pSession)
  {
    // #TODO_XR flush command queue
    xrDestroySession(m_pSession);
    m_pSession = XR_NULL_HANDLE;
  }

  DeinitGraphicsPlugin();
  DeinitInitDebugMessenger();
}

XrResult WOpenXR::InitGraphicsPlugin()
{
  if (!m_pGraphicsBinding)
  {
    WLog::Error("No graphics binding available");
    return XR_ERROR_GRAPHICS_DEVICE_INVALID;
  }

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  XrResult result = m_pGraphicsBinding->Initialize(m_pInstance, m_SystemId, pDevice);
  if (result != XR_SUCCESS)
  {
    WLog::Error("Failed to initialize graphics binding: {}", (int)result);
    return result;
  }

  WLog::Info("OpenXR graphics binding initialized: {}", m_pGraphicsBinding->GetName());
  return XR_SUCCESS;
}

void WOpenXR::DeinitGraphicsPlugin()
{
  if (m_pGraphicsBinding)
  {
    m_pGraphicsBinding->Deinitialize();
  }
}

XrResult WOpenXR::InitDebugMessenger()
{
  XrDebugUtilsMessengerCreateInfoEXT create_info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  create_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.userCallback = xrDebugCallback;

  XR_SUCCEED_OR_CLEANUP_LOG(m_Extensions.pfn_xrCreateDebugUtilsMessengerEXT(m_pInstance, &create_info, &m_pDebugMessenger), DeinitInitDebugMessenger);

  return XrResult::XR_SUCCESS;
}

void WOpenXR::DeinitInitDebugMessenger()
{
  if (m_pDebugMessenger != XR_NULL_HANDLE)
  {
    XR_LOG_ERROR(m_Extensions.pfn_xrDestroyDebugUtilsMessengerEXT(m_pDebugMessenger));
    m_pDebugMessenger = XR_NULL_HANDLE;
  }
}

void WOpenXR::BeforeUpdatePlugins()
{
  W_PROFILE_SCOPE("BeforeUpdatePlugins");
  // Make sure the main camera component is set to stereo mode.
  if (WWorld* pWorld = GetWorld())
  {
    W_LOCK(pWorld->GetWriteMarker());
    auto* pCCM = pWorld->GetComponentManager<WCameraComponentManager>();
    if (pCCM)
    {
      if (WCameraComponent* pCameraComponent = pCCM->GetCameraByUsageHint(WCameraUsageHint::MainView))
      {
        pCameraComponent->SetCameraMode(WCameraMode::Stereo);
      }
    }
  }

  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER, nullptr};

  while (xrPollEvent(m_pInstance, &event) == XR_SUCCESS)
  {
    switch (event.type)
    {
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        m_pInput->UpdateCurrentInteractionProfile();
      }
      break;
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const XrEventDataSessionStateChanged& session_state_changed_event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        m_SessionState = session_state_changed_event.state;
        switch (m_SessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = m_PrimaryViewConfigurationType;
            if (xrBeginSession(m_pSession, &sessionBeginInfo) == XR_SUCCESS)
            {
              m_bSessionRunning = true;
            }
            break;
          }
          case XR_SESSION_STATE_STOPPING:
          {
            m_bSessionRunning = false;
            if (xrEndSession(m_pSession) != XR_SUCCESS)
            {
              // TODO log
            }
            break;
          }
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart because user closed this session.
            m_bExitRenderLoop = true;
            m_bRequestRestart = false;
            break;
          }
          case XR_SESSION_STATE_LOSS_PENDING:
          {
            // Poll for a new systemId
            m_bExitRenderLoop = true;
            m_bRequestRestart = true;
            break;
          }
          default:
            break;
        }
      }
      break;
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        const XrEventDataInstanceLossPending& instance_loss_pending_event = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);
        m_bExitRenderLoop = true;
        m_bRequestRestart = false;
      }
      break;
      default:
        break;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
  }

  if (m_bExitRenderLoop)
  {
    DeinitSession();
    DeinitSystem();
    if (m_bRequestRestart)
    {
      // Try to re-init session
      XrResult res = InitSystem();
      if (res != XR_SUCCESS)
      {
        return;
      }
      res = InitSession();
      if (res != XR_SUCCESS)
      {
        DeinitSystem();
        return;
      }
    }
  }
  // #TODO exit render loop and restart logic not fully implemented.
}

void WOpenXR::UpdatePoses()
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  W_PROFILE_SCOPE("UpdatePoses");
  m_ViewState = XrViewState{XR_TYPE_VIEW_STATE};
  WUInt32 viewCapacityInput = 2;
  WUInt32 viewCountOutput;

  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = m_PrimaryViewConfigurationType;
  viewLocateInfo.displayTime = m_FrameState.predictedDisplayTime;
  viewLocateInfo.space = GetBaseSpace();
  m_Views[0].type = XR_TYPE_VIEW;
  m_Views[1].type = XR_TYPE_VIEW;
  XrFovf previousFov[2];
  previousFov[0] = m_Views[0].fov;
  previousFov[1] = m_Views[1].fov;

  XrResult res = xrLocateViews(m_pSession, &viewLocateInfo, &m_ViewState, viewCapacityInput, &viewCountOutput, m_Views);

  if (res == XR_SUCCESS)
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = ((m_ViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) && (m_ViewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT));
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = m_pInput->m_DeviceState[0].m_bGripPoseIsValid;
  }
  else
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = false;
  }

  // Needed as workaround for broken XR runtimes.
  auto FovIsNull = [](const XrFovf& fov)
  {
    return fov.angleLeft == 0.0f && fov.angleRight == 0.0f && fov.angleDown == 0.0f && fov.angleUp == 0.0f;
  };

  auto IdentityFov = [](XrFovf& fov)
  {
    fov.angleLeft = -WAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleRight = WAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleUp = WAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleDown = -WAngle::MakeFromDegree(45.0f).GetRadian();
  };

  if (FovIsNull(m_Views[0].fov) || FovIsNull(m_Views[1].fov))
  {
    IdentityFov(m_Views[0].fov);
    IdentityFov(m_Views[1].fov);
  }

  m_bProjectionChanged = WMemoryUtils::Compare(&previousFov[0], &m_Views[0].fov, 1) != 0 || WMemoryUtils::Compare(&previousFov[1], &m_Views[1].fov, 1) != 0;

  for (WUInt32 uiEyeIndex : {0, 1})
  {
    WQuat rot = WOpenXRConversionUtils::ConvertOrientation(m_Views[uiEyeIndex].pose.orientation);
    if (!rot.IsValid())
    {
      m_Views[uiEyeIndex].pose.orientation = XrQuaternionf{0, 0, 0, 1};
    }
  }

  UpdateCamera();
  m_pInput->UpdateActions();
  if (m_pHandTracking)
  {
    m_pHandTracking->UpdateJointTransforms();
  }
}

void WOpenXR::UpdateCamera()
{
  if (!m_pCameraToSynchronize)
  {
    return;
  }
  // Update camera projection
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter() || m_bProjectionChanged)
  {
    m_bProjectionChanged = false;
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;
    auto CreateProjection = [](const XrView& view, WCamera* cam)
    {
      return WGraphicsUtils::CreatePerspectiveProjectionMatrix(WMath::Tan(WAngle::MakeFromRadian(view.fov.angleLeft)) * cam->GetNearPlane(), WMath::Tan(WAngle::MakeFromRadian(view.fov.angleRight)) * cam->GetNearPlane(), WMath::Tan(WAngle::MakeFromRadian(view.fov.angleDown)) * cam->GetNearPlane(),
        WMath::Tan(WAngle::MakeFromRadian(view.fov.angleUp)) * cam->GetNearPlane(), cam->GetNearPlane(), cam->GetFarPlane());
    };

    // Update projection with newest near/ far values. If not sync camera is set, just use the last value from XR
    // camera.
    const WMat4 projLeft = CreateProjection(m_Views[0], m_pCameraToSynchronize);
    const WMat4 projRight = CreateProjection(m_Views[1], m_pCameraToSynchronize);
    m_pCameraToSynchronize->SetStereoProjection(projLeft, projRight, fAspectRatio);
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter();
  }

  // Update camera view
  {
    WTransform add;
    add.SetIdentity();
    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      if (const WWorld* pWorld = pView->GetWorld())
      {
        W_LOCK(pWorld->GetReadMarker());
        if (const WStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<WStageSpaceComponentManager>())
        {
          if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            WEnum<WXRStageSpace> stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      // Update device state (average of both eyes).
      const WQuat rot = WQuat::MakeSlerp(WOpenXRConversionUtils::ConvertOrientation(m_Views[0].pose.orientation), WOpenXRConversionUtils::ConvertOrientation(m_Views[1].pose.orientation), 0.5f);
      const WVec3 pos = WMath::Lerp(WOpenXRConversionUtils::ConvertPosition(m_Views[0].pose.position), WOpenXRConversionUtils::ConvertPosition(m_Views[1].pose.position), 0.5f);

      m_pInput->m_DeviceState[0].m_vGripPosition = pos;
      m_pInput->m_DeviceState[0].m_qGripRotation = rot;
      m_pInput->m_DeviceState[0].m_vAimPosition = pos;
      m_pInput->m_DeviceState[0].m_qAimRotation = rot;
      m_pInput->m_DeviceState[0].m_Type = WXRDeviceType::HMD;
      m_pInput->m_DeviceState[0].m_bGripPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bAimPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bDeviceIsConnected = true;
    }

    // Set view matrix
    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      const WMat4 mStageTransform = add.GetAsMat4();
      const WMat4 poseLeft = mStageTransform * WOpenXRConversionUtils::ConvertPoseToMatrix(m_Views[0].pose);
      const WMat4 poseRight = mStageTransform * WOpenXRConversionUtils::ConvertPoseToMatrix(m_Views[1].pose);

      // W Forward is +X, need to add this to align the forward projection
      const WMat4 viewMatrix = WGraphicsUtils::CreateLookAtViewMatrix(WVec3::MakeZero(), WVec3(1, 0, 0), WVec3(0, 0, 1));
      const WMat4 mViewTransformLeft = viewMatrix * poseLeft.GetInverse();
      const WMat4 mViewTransformRight = viewMatrix * poseRight.GetInverse();

      m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, WCameraEye::Left);
      m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, WCameraEye::Right);
    }
  }
}

void WOpenXR::BeginFrame()
{
  if (m_hView.IsInvalidated() || !m_bSessionRunning)
    return;

  W_PROFILE_SCOPE("OpenXrBeginFrame");
  {
    W_PROFILE_SCOPE("xrWaitFrame");
    m_FrameWaitInfo = XrFrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    m_FrameState = XrFrameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_pSession, &m_FrameWaitInfo, &m_FrameState);
    if (result != XR_SUCCESS)
    {
      m_bRenderInProgress = false;
      return;
    }
  }
  {
    W_PROFILE_SCOPE("xrBeginFrame");
    m_FrameBeginInfo = XrFrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = xrBeginFrame(m_pSession, &m_FrameBeginInfo);
    if (result == XR_FRAME_DISCARDED)
    {
      WLog::Error("OpenXR call '{0}' failed with: XR_FRAME_DISCARDED", "xrBeginFrame");
    }
    else if (result != XR_SUCCESS)
    {
      WLog::Error("OpenXR call '{0}' failed with: {1}", "xrBeginFrame", (int)result);
      m_bRenderInProgress = false;
      return;
    }
  }

  // #TODO_XR Swap chain acquire here?

  UpdatePoses();

  // This will update the extracted view from last frame with the new data we got
  // this frame just before starting to render.
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    pView->UpdateViewData(WRenderWorld::GetDataIndexForRendering());
  }

  if (m_pCompanion)
  {
    m_pCompanion->CompanionViewBeginFrame();
  }
  m_bRenderInProgress = true;
}

void WOpenXR::EndRender()
{
  const WGALOpenXRSwapChain* pSwapChain = static_cast<const WGALOpenXRSwapChain*>(WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!m_bRenderInProgress || !pSwapChain)
    return;
}

WGALTextureHandle WOpenXR::GetCurrentTexture()
{
  const WGALOpenXRSwapChain* pSwapChain = static_cast<const WGALOpenXRSwapChain*>(WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!pSwapChain)
    return WGALTextureHandle();

  return pSwapChain->m_hColorRT;
}

void WOpenXR::EndFrame()
{
  const WGALOpenXRSwapChain* pSwapChain = static_cast<const WGALOpenXRSwapChain*>(WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));

  if (!m_bRenderInProgress || !pSwapChain)
    return;
  /// NOTE: (Only Applies When Tracy is Enabled.)Tracy Seems to declare Timers in the same scope, so dual profile macros can throw: '__tracy_scoped_zone' : redefinition; multitple initalization, so we must scope the two events.
  {
    W_PROFILE_SCOPE("OpenXrEndFrame");
    for (uint32_t i = 0; i < 2; i++)
    {
      m_ProjectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
      m_ProjectionLayerViews[i].pose = m_Views[i].pose;
      m_ProjectionLayerViews[i].fov = m_Views[i].fov;
      m_ProjectionLayerViews[i].subImage.swapchain = pSwapChain->GetColorSwapchain();
      m_ProjectionLayerViews[i].subImage.imageRect.offset = {0, 0};
      m_ProjectionLayerViews[i].subImage.imageRect.extent = {(WInt32)m_Info.m_vEyeRenderTargetSize.width, (WInt32)m_Info.m_vEyeRenderTargetSize.height};
      m_ProjectionLayerViews[i].subImage.imageArrayIndex = i;

      if (m_Extensions.m_bDepthComposition && m_pCameraToSynchronize)
      {
        m_DepthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
        m_DepthLayerViews[i].minDepth = 0;
        m_DepthLayerViews[i].maxDepth = 1;
        m_DepthLayerViews[i].nearZ = m_pCameraToSynchronize->GetNearPlane();
        m_DepthLayerViews[i].farZ = m_pCameraToSynchronize->GetFarPlane();
        m_DepthLayerViews[i].subImage.swapchain = pSwapChain->GetDepthSwapchain();
        m_DepthLayerViews[i].subImage.imageRect.offset = {0, 0};
        m_DepthLayerViews[i].subImage.imageRect.extent = {(WInt32)m_Info.m_vEyeRenderTargetSize.width, (WInt32)m_Info.m_vEyeRenderTargetSize.height};
        m_DepthLayerViews[i].subImage.imageArrayIndex = i;

        m_ProjectionLayerViews[i].next = &m_DepthLayerViews[i];
      }
    }
  }

  m_Layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
  m_Layer.space = GetBaseSpace();
  m_Layer.viewCount = 2;
  m_Layer.views = m_ProjectionLayerViews;

  WHybridArray<XrCompositionLayerBaseHeader*, 1> layers;
  layers.PushBack(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_Layer));

  // Submit the composition layers for the predicted display time.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = m_FrameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = m_BlendMode;
  frameEndInfo.layerCount = layers.GetCapacity();
  frameEndInfo.layers = layers.GetData();

  W_PROFILE_SCOPE("xrEndFrame");
  XR_LOG_ERROR(xrEndFrame(m_pSession, &frameEndInfo));
}

void WOpenXR::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::Type::BeforeInit)
  {
    if (InitInstance(e.m_pDevice).Failed())
    {
      WLog::Error("OpenXR: InitInstance failed");
      return;
    }
    XrResult res = InitSystem();
    if (res != XR_SUCCESS)
    {
      WLog::Error("OpenXR: InitSystem failed: {}", res);
      return;
    }
  }
  else if (e.m_Type == WGALDeviceEvent::Type::AfterShutdown)
  {
    DeinitSystem();
    DeinitInstance();
  }

  if (!m_pSession)
    return;

  // Begin frame and end frame need to be encompassing all workload, XR and otherwise as xrWaitFrame will use this time interval to decide when to wake up the application.
  if (e.m_Type == WGALDeviceEvent::Type::BeforeBeginFrame)
  {
    BeginFrame();
  }
  else if (e.m_Type == WGALDeviceEvent::Type::AfterEndFrame)
  {
    EndFrame();
  }
}

void WOpenXR::OnRenderWorldEvent(const WRenderWorldRenderEvent& e)
{
  if (e.m_Type == WRenderWorldRenderEvent::Type::EndRender)
  {
    EndRender();
  }
}

void WOpenXR::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (e.m_Type == WGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    BeforeUpdatePlugins();
  }
}

void WOpenXR::SetStageSpace(WXRStageSpace::Enum space)
{
  m_StageSpace = space;
}

void WOpenXR::SetHMDCamera(WCamera* pCamera)
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_pCameraToSynchronize->SetCameraMode(WCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }
}

WWorld* WOpenXR::GetWorld()
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    return pView->GetWorld();
  }
  return nullptr;
}


W_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSingleton);


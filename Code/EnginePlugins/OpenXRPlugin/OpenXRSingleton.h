#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WOpenXRInputDevice;
class WOpenXRSpatialAnchors;
class WOpenXRHandTracking;
class WOpenXRGraphicsBinding;
class WWindowOutputTargetXR;
struct WGameApplicationExecutionEvent;
struct WRenderWorldRenderEvent;
class WGALDevice;

W_DEFINE_AS_POD_TYPE(XrViewConfigurationView);
W_DEFINE_AS_POD_TYPE(XrEnvironmentBlendMode);
W_DEFINE_AS_POD_TYPE(XrExtensionProperties);
W_DEFINE_AS_POD_TYPE(XrApiLayerProperties);

class W_OPENXRPLUGIN_DLL WOpenXR : public WXRInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WOpenXR, WXRInterface);

public:
  WOpenXR();
  ~WOpenXR();

  XrInstance GetInstance() const { return m_pInstance; }
  uint64_t GetSystemId() const { return m_SystemId; }
  XrSession GetSession() const { return m_pSession; }
  XrViewConfigurationType GetViewType() const { return m_PrimaryViewConfigurationType; }
  bool GetDepthComposition() const;

  /// Returns the graphics binding interface (D3D11, Vulkan, etc.)
  WOpenXRGraphicsBinding* GetGraphicsBinding() const { return m_pGraphicsBinding.Borrow(); }

  virtual bool IsHmdPresent() const override;

  virtual WResult Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const WHMDInfo& GetHmdInfo() const override;
  virtual WXRInputDevice& GetXRInput() const override;

  virtual WGALTextureHandle GetCurrentTexture() override;

  virtual WRegisteredWndHandle CreateXRWindow(WView* pView, WGALMSAASampleCount::Enum msaaCount = WGALMSAASampleCount::None,
    WUniquePtr<WWindowBase> pCompanionWindow = nullptr, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  virtual void OnActorDestroyed() override;
  virtual bool SupportsCompanionView() override;

  XrSpace GetBaseSpace() const;

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, OpenXR);

  void OnEngineStartup();
  void OnEngineShutdown();
  XrResult SelectExtensions(WHybridArray<const char*, 6>& extensions);
  XrResult SelectLayers(WHybridArray<const char*, 6>& layers);
  WResult InitInstance(WGALDevice* pDevice);
  void DeinitInstance();
  XrResult InitSystem();
  void DeinitSystem();
  XrResult InitSession();
  void DeinitSession();
  XrResult InitGraphicsPlugin();
  void DeinitGraphicsPlugin();
  XrResult InitDebugMessenger();
  void DeinitInitDebugMessenger();

  void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const WGALDeviceEvent& e);
  void OnRenderWorldEvent(const WRenderWorldRenderEvent& e);

  void BeforeUpdatePlugins();
  void UpdatePoses();
  void UpdateCamera();
  void BeginFrame();
  void EndRender();
  void EndFrame();

  void SetStageSpace(WXRStageSpace::Enum space);
  void SetHMDCamera(WCamera* pCamera);

  WWorld* GetWorld();

private:
  friend class WOpenXRInputDevice;
  friend class WOpenXRSpatialAnchors;
  friend class WOpenXRHandTracking;
  friend class WOpenXRRemoting;
  friend class WGALOpenXRSwapChain;

  struct Extensions
  {
    bool m_bValidation = false;
    bool m_bDebugUtils = false;
    PFN_xrCreateDebugUtilsMessengerEXT pfn_xrCreateDebugUtilsMessengerEXT = nullptr;
    PFN_xrDestroyDebugUtilsMessengerEXT pfn_xrDestroyDebugUtilsMessengerEXT = nullptr;

    bool m_bDepthComposition = false;

    bool m_bUnboundedReferenceSpace = false;

    bool m_bSpatialAnchor = false;
    PFN_xrCreateSpatialAnchorMSFT pfn_xrCreateSpatialAnchorMSFT = nullptr;
    PFN_xrCreateSpatialAnchorSpaceMSFT pfn_xrCreateSpatialAnchorSpaceMSFT = nullptr;
    PFN_xrDestroySpatialAnchorMSFT pfn_xrDestroySpatialAnchorMSFT = nullptr;

    bool m_bHandInteraction = false;

    bool m_bHandTracking = false;
    PFN_xrCreateHandTrackerEXT pfn_xrCreateHandTrackerEXT = nullptr;
    PFN_xrDestroyHandTrackerEXT pfn_xrDestroyHandTrackerEXT = nullptr;
    PFN_xrLocateHandJointsEXT pfn_xrLocateHandJointsEXT = nullptr;

    bool m_bHandTrackingMesh = false;
    PFN_xrCreateHandMeshSpaceMSFT pfn_xrCreateHandMeshSpaceMSFT = nullptr;
    PFN_xrUpdateHandMeshMSFT pfn_xrUpdateHandMeshMSFT = nullptr;

    bool m_bHolographicWindowAttachment = false;
  };

  // Instance
  XrInstance m_pInstance = XR_NULL_HANDLE;
  Extensions m_Extensions;

  // System
  uint64_t m_SystemId = XR_NULL_SYSTEM_ID;

  // Session
  XrSession m_pSession = XR_NULL_HANDLE;
  XrSpace m_pSceneSpace = XR_NULL_HANDLE;
  XrSpace m_pLocalSpace = XR_NULL_HANDLE;
  WEventSubscriptionID m_ExecutionEventsId = 0;
  WEventSubscriptionID m_BeginRenderEventsId = 0;
  WEventSubscriptionID m_GALdeviceEventsId = 0;
  WEventSubscriptionID m_RenderWorldEventId = 0;
  XrDebugUtilsMessengerEXT m_pDebugMessenger = XR_NULL_HANDLE;

  // Graphics binding (abstracts D3D11, Vulkan, etc.)
  XrEnvironmentBlendMode m_BlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  WUniquePtr<WOpenXRGraphicsBinding> m_pGraphicsBinding;
  XrFormFactor m_FormFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  XrViewConfigurationType m_PrimaryViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

  WGALSwapChainHandle m_hSwapChain;

  // Views
  XrViewState m_ViewState{XR_TYPE_VIEW_STATE};
  XrView m_Views[2];
  bool m_bProjectionChanged = true;
  XrCompositionLayerProjection m_Layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  XrCompositionLayerProjectionView m_ProjectionLayerViews[2];
  XrCompositionLayerDepthInfoKHR m_DepthLayerViews[2];

  // State
  bool m_bSessionRunning = false;
  bool m_bExitRenderLoop = false;
  bool m_bRequestRestart = false;
  bool m_bRenderInProgress = false;
  XrSessionState m_SessionState{XR_SESSION_STATE_UNKNOWN};

  XrFrameWaitInfo m_FrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState m_FrameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo m_FrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};

  // XR interface state
  WHMDInfo m_Info;
  mutable WUniquePtr<WOpenXRInputDevice> m_pInput;
  WUniquePtr<WOpenXRSpatialAnchors> m_pAnchors;
  WUniquePtr<WOpenXRHandTracking> m_pHandTracking;

  WCamera* m_pCameraToSynchronize = nullptr;
  WEnum<WXRStageSpace> m_StageSpace;
  WUInt32 m_uiSettingsModificationCounter = 0;
  WViewHandle m_hView;

  WWindowOutputTargetXR* m_pCompanion = nullptr;
};

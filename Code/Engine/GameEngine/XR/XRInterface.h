#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/WindowManager.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;

class WViewHandle;
class WCamera;
class WGALTextureHandle;
class WWorld;
class WView;
class WXRInputDevice;
class WWindowBase;
class WWindowOutputTargetBase;
struct WGALMSAASampleCount;

/// XR singleton interface. Needs to be initialized to be used for VR or AR purposes.
///
/// To be used in a project the project needs to have an enabled WXRConfig with a
/// set render pipeline in the platform profile.
/// To then use the interface, Initialize must be called first and on success CreateActor.
/// Everything else is optional.
/// Aquire interface via WSingletonRegistry::GetSingletonInstance<WXRInterface>().
class WXRInterface
{
public:
  /// Returns whether an HMD is available. Can be used to decide
  /// whether it makes sense to call Initialize at all.
  virtual bool IsHmdPresent() const = 0;

  /// \name Setup
  ///@{

  /// Initializes the XR system. This can be quite time consuming
  /// as it will generally start supporting applications needed to run
  /// and start up the HMD if it went to sleep.
  virtual WResult Initialize() = 0;
  /// Shuts down the XR system again.
  virtual void Deinitialize() = 0;
  /// Returns whether the XR system is initialized.
  virtual bool IsInitialized() const = 0;

  ///@}
  /// \name Devices
  ///@{

  /// Returns general HMD information.
  virtual const WHMDInfo& GetHmdInfo() const = 0;

  /// Returns the XR input device.
  virtual WXRInputDevice& GetXRInput() const = 0;

  ///@}
  /// \name View
  ///@{

  /// Returns true if a companion window can be passed into CreateActor.
  virtual bool SupportsCompanionView() = 0;

  /// Creates a XR actor by trying to startup an XR session.
  ///
  /// If SupportsCompanionView is true (VR only), a normal window and window output can be passed in.
  /// The window will be used to blit the VR output into the window.
  virtual WRegisteredWndHandle CreateXRWindow(WView* pView, WGALMSAASampleCount::Enum msaaCount = WGALMSAASampleCount::None, WUniquePtr<WWindowBase> pCompanionWindow = nullptr, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) = 0;

  ///@}
  /// \name Internal
  ///@{

  /// Called by WWindowOutputTargetXR::RenderCompanionView
  /// Returns the color texture to be used by the companion view if enabled, otherwise an invalid handle.
  virtual WGALTextureHandle GetCurrentTexture() = 0;

  /// Called when the actor created by 'CreateActor' is destroyed.
  virtual void OnActorDestroyed() = 0;

  ///@}
};

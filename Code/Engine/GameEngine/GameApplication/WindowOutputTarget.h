#pragma once

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>

struct WGALDeviceEvent;
class WRenderGraph;

/// Creates a swapchain and keeps it up to date with the window.
///
/// If the window is resized or WGameApplication::cvar_AppVSync changes and onSwapChainChanged is valid, the swapchain is destroyed and recreated. It is up the the application to respond to the OnSwapChainChanged callback and update any references to the swap-chain, e.g. uses in WView or uses as render targets in WGALRenderTargetSetup.
/// If onSwapChainChanged is not set, the swapchain will not be re-created and it is up to the application to manage the swapchain and react to window changes.
class W_GAMEENGINE_DLL WWindowOutputTargetGAL : public WWindowOutputTargetBase
{
public:
  using OnSwapChainChanged = WDelegate<void(WGALSwapChainHandle hSwapChain, WSizeU32 size)>;
  WWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged = {});
  ~WWindowOutputTargetGAL();

  void CreateSwapchain(const WGALWindowSwapChainCreationDescription& desc);

  virtual void PresentImage(bool bEnableVSync) override;
  virtual void AcquireImage() override;
  virtual WResult StartCaptureImage() override;
  virtual WEnum<WCaptureImageResult> WaitCaptureImage(WImage& out_image) override;

  WGALSwapChainHandle m_hSwapChain;

private:
  void SwapChainUpdatedEventHandler(const WGALSwapChain* pSwapChain);
  void OnRenderEvent(const WGALDeviceEvent& e);

  OnSwapChainChanged m_OnSwapChainChanged;
  WSizeU32 m_Size = WSizeU32(0, 0);
  WGALWindowSwapChainCreationDescription m_CurrentDesc;

  // Capture image functionality
  WGALReadbackTextureHelper m_Readback;
  WSharedPtr<WRenderGraph> m_pRenderGraph;
  bool m_bCaptureRequested = false;
  bool m_bCaptureInFlight = false;
  WGALTextureCreationDescription m_CaptureBackbufferDesc; ///< Needs to be stored as the swapchain could be resized between the two capture calls.
};

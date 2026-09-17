#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

#include <RendererFoundation/Resources/ReadbackHelper.h>

class WXRInterface;
class WRenderGraph;

/// XR Window base implementation. Optionally wraps a companion window.
class W_GAMEENGINE_DLL WWindowXR : public WWindowBase
{
public:
  WWindowXR(WXRInterface* pVrInterface, WUniquePtr<WWindowBase> pCompanionWindow);
  virtual ~WWindowXR();

  virtual WSizeU32 GetClientAreaSize() const override;

  virtual WWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsVisible() const override { return true; }
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

  /// Returns the companion window if present.
  const WWindowBase* GetCompanionWindow() const;

private:
  WXRInterface* m_pVrInterface = nullptr;
  WUniquePtr<WWindowBase> m_pCompanionWindow;
  WAtomicInteger32 m_iReferenceCount = 0;
};

/// XR Window output target base implementation. Optionally wraps a companion window output target.
class W_GAMEENGINE_DLL WWindowOutputTargetXR : public WWindowOutputTargetBase
{
public:
  WWindowOutputTargetXR(WXRInterface* pVrInterface, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutputTarget);
  ~WWindowOutputTargetXR();

  virtual void AcquireImage() override {}
  virtual void PresentImage(bool bEnableVSync) override;
  void CompanionViewBeginFrame(bool bThrottleCompanionView = true);
  virtual WResult StartCaptureImage() override;
  virtual WEnum<WCaptureImageResult> WaitCaptureImage(WImage& out_image) override;

  /// Returns the companion window output target if present.
  const WWindowOutputTargetBase* GetCompanionWindowOutputTarget() const;

private:
  void RenderCompanionView();
  void OnGALEvent(const WGALDeviceEvent& e);

  WXRInterface* m_pXrInterface = nullptr;
  WTime m_LastPresent;
  WUniquePtr<WWindowOutputTargetGAL> m_pCompanionWindowOutputTarget;
  WConstantBufferStorageHandle m_hCompanionConstantBuffer;
  WShaderResourceHandle m_hCompanionShader;
  bool m_bRender = false;
  WSharedPtr<WRenderGraph> m_pRenderGraph;
  bool m_bCaptureRequested = false;
  bool m_bCaptureInFlight = false;
  WGALReadbackTextureHelper m_Readback;
  WGALTextureCreationDescription m_CaptureBackbufferDesc;
};

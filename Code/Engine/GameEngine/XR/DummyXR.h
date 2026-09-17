#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Pipeline/Declarations.h>

struct WGALDeviceEvent;
struct WGameApplicationExecutionEvent;
class WWindowOutputTargetXR;

class W_GAMEENGINE_DLL WDummyXRInput : public WXRInputDevice
{

public:
  void GetDeviceList(WHybridArray<WXRDeviceID, 64>& out_devices) const override;
  WXRDeviceID GetDeviceIDByType(WXRDeviceType::Enum type) const override;
  const WXRDeviceState& GetDeviceState(WXRDeviceID deviceID) const override;
  WString GetDeviceName(WXRDeviceID deviceID) const override;
  WBitflags<WXRDeviceFeatures> GetDeviceFeatures(WXRDeviceID deviceID) const override;

protected:
  void InitializeDevice() override;
  void UpdateInputSlotValues() override;
  void RegisterInputSlots() override;

protected:
  friend class WDummyXR;

  WXRDeviceState m_DeviceState[1];
};

class W_GAMEENGINE_DLL WDummyXR : public WXRInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WDummyXR, WXRInterface);

public:
  WDummyXR();
  ~WDummyXR() = default;

  bool IsHmdPresent() const override;
  WResult Initialize() override;
  void Deinitialize() override;
  bool IsInitialized() const override;
  const WHMDInfo& GetHmdInfo() const override;
  WXRInputDevice& GetXRInput() const override;
  bool SupportsCompanionView() override;
  WRegisteredWndHandle CreateXRWindow(WView* pView, WGALMSAASampleCount::Enum msaaCount = WGALMSAASampleCount::None, WUniquePtr<WWindowBase> pCompanionWindow = nullptr, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  WGALTextureHandle GetCurrentTexture() override;
  void OnActorDestroyed() override;
  void GALDeviceEventHandler(const WGALDeviceEvent& e);
  void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);

protected:
  float m_fHeadHeight = 1.7f;
  float m_fEyeOffset = 0.05f;


  WHMDInfo m_Info;
  mutable WDummyXRInput m_Input;
  bool m_bInitialized = false;

  WEventSubscriptionID m_GALdeviceEventsId = 0;
  WEventSubscriptionID m_ExecutionEventsId = 0;

  WCamera* m_pCameraToSynchronize = nullptr;
  WEnum<WXRStageSpace> m_StageSpace = WXRStageSpace::Seated;

  WViewHandle m_hView;
  WGALTextureHandle m_hColorRT;
  WGALTextureHandle m_hDepthRT;

  WWindowOutputTargetXR* m_pCompanion = nullptr;
};

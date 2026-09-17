#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

class WOpenXR;

W_DEFINE_AS_POD_TYPE(XrActionSuggestedBinding);
W_DEFINE_AS_POD_TYPE(XrActiveActionSet);

class W_OPENXRPLUGIN_DLL WOpenXRInputDevice : public WXRInputDevice
{
  W_ADD_DYNAMIC_REFLECTION(WOpenXRInputDevice, WXRInputDevice);

public:
  void GetDeviceList(WHybridArray<WXRDeviceID, 64>& out_devices) const override;
  WXRDeviceID GetDeviceIDByType(WXRDeviceType::Enum type) const override;
  const WXRDeviceState& GetDeviceState(WXRDeviceID deviceID) const override;
  WString GetDeviceName(WXRDeviceID deviceID) const override;
  WBitflags<WXRDeviceFeatures> GetDeviceFeatures(WXRDeviceID deviceID) const override;

private:
  friend class WOpenXR;
  struct Bind
  {
    XrAction action;
    const char* szPath;
  };

  struct Action
  {
    WXRDeviceFeatures::Enum m_Feature;
    XrAction m_Action;
    WString m_sKey[2];
  };

  struct Vec2Action
  {
    Vec2Action(WXRDeviceFeatures::Enum feature, XrAction pAction, WStringView sLeft, WStringView sRight);
    WXRDeviceFeatures::Enum m_Feature;
    XrAction m_Action;
    WString m_sKey_negx[2];
    WString m_sKey_posx[2];
    WString m_sKey_negy[2];
    WString m_sKey_posy[2];
  };

  WOpenXRInputDevice(WOpenXR* pOpenXR);
  XrResult CreateActions(XrSession session, XrSpace m_sceneSpace);
  void DestroyActions();

  XrPath CreatePath(const char* szPath);
  XrResult CreateAction(WXRDeviceFeatures::Enum feature, const char* actionName, XrActionType actionType, XrAction& out_action);
  XrResult SuggestInteractionProfileBindings(const char* szInteractionProfile, const char* szNiceName, WArrayPtr<Bind> bindings);
  XrResult AttachSessionActionSets(XrSession session);
  XrResult UpdateCurrentInteractionProfile();

  void InitializeDevice() override;
  void RegisterInputSlots() override;
  void UpdateInputSlotValues() override {}

  XrResult UpdateActions();
  void UpdateControllerState();

private:
  WOpenXR* m_pOpenXR = nullptr;
  XrInstance m_pInstance = XR_NULL_HANDLE;
  XrSession m_pSession = XR_NULL_HANDLE;

  WXRDeviceState m_DeviceState[3]; // Hard-coded for now
  WString m_sActiveProfile[3];
  WBitflags<WXRDeviceFeatures> m_SupportedFeatures[3];
  const WInt8 m_iLeftControllerDeviceID = 1;
  const WInt8 m_iRightControllerDeviceID = 2;

  XrActionSet m_pActionSet = XR_NULL_HANDLE;
  WHashTable<WUInt64, WString> m_InteractionProfileToNiceName;

  WStaticArray<const char*, 2> m_SubActionPrefix;
  WStaticArray<XrPath, 2> m_SubActionPath = {};

  WHybridArray<Action, 4> m_BooleanActions;
  WHybridArray<Action, 4> m_FloatActions;
  WHybridArray<Vec2Action, 4> m_Vec2Actions;
  WHybridArray<Action, 4> m_PoseActions;

  XrSpace m_gripSpace[2] = {};
  XrSpace m_aimSpace[2] = {};
};

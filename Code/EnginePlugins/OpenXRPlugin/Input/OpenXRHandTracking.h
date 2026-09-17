#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class WOpenXR;

class W_OPENXRPLUGIN_DLL WOpenXRHandTracking : public WXRHandTrackingInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WOpenXRHandTracking, WXRHandTrackingInterface);

public:
  static bool IsHandTrackingSupported(WOpenXR* pOpenXR);

public:
  WOpenXRHandTracking(WOpenXR* pOpenXR);
  ~WOpenXRHandTracking();

  HandPartTrackingState TryGetBoneTransforms(
    WEnum<WXRHand> hand, WEnum<WXRHandPart> handPart, WEnum<WXRTransformSpace> space, WDynamicArray<WXRHandBone>& out_bones) override;

  void UpdateJointTransforms();

private:
  friend class WOpenXR;

  struct JointData
  {
    W_DECLARE_POD_TYPE();
    WXRHandBone m_Bone;
    bool m_bValid;
  };

  WOpenXR* m_pOpenXR = nullptr;
  XrHandTrackerEXT m_HandTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  XrHandJointLocationEXT m_JointLocations[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT m_JointVelocities[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT m_Locations[2]{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
  XrHandJointVelocitiesEXT m_Velocities[2]{XR_TYPE_HAND_JOINT_VELOCITIES_EXT};

  WStaticArray<JointData, XR_HAND_JOINT_LITTLE_TIP_EXT + 1> m_JointData[2];
  WStaticArray<WUInt32, 6> m_HandParts[WXRHandPart::Little + 1];
};

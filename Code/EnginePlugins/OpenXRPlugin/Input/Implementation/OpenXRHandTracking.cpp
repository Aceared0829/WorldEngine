#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/Input/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/Utils/OpenXRConversionUtils.h>

W_IMPLEMENT_SINGLETON(WOpenXRHandTracking);

bool WOpenXRHandTracking::IsHandTrackingSupported(WOpenXR* pOpenXR)
{
  XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties};
  XrResult res = xrGetSystemProperties(pOpenXR->m_pInstance, pOpenXR->m_SystemId, &systemProperties);
  if (res == XrResult::XR_SUCCESS)
  {
    return handTrackingSystemProperties.supportsHandTracking;
  }
  return false;
}

WOpenXRHandTracking::WOpenXRHandTracking(WOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
  for (WUInt32 uiSide : {0, 1})
  {
    const XrHandEXT uiHand = uiSide == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
    XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    createInfo.hand = uiHand;
    XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrCreateHandTrackerEXT(pOpenXR->m_pSession, &createInfo, &m_HandTracker[uiSide]));

    m_Locations[uiSide].type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
    m_Locations[uiSide].next = &m_Velocities[uiSide];
    m_Locations[uiSide].jointCount = XR_HAND_JOINT_COUNT_EXT;
    m_Locations[uiSide].jointLocations = m_JointLocations[uiSide];
    WMemoryUtils::ZeroFill(&m_JointLocations[uiSide][0], XR_HAND_JOINT_COUNT_EXT);

    m_Velocities[uiSide].type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
    m_Velocities[uiSide].jointCount = XR_HAND_JOINT_COUNT_EXT;
    m_Velocities[uiSide].jointVelocities = m_JointVelocities[uiSide];
    WMemoryUtils::ZeroFill(&m_JointVelocities[uiSide][0], XR_HAND_JOINT_COUNT_EXT);

    m_JointData[uiSide].SetCount(XR_HAND_JOINT_LITTLE_TIP_EXT + 1);
    for (WUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
    {
      m_JointData[uiSide][i].m_Bone.m_Transform.SetIdentity();
      m_JointVelocities[uiSide][i].velocityFlags = XR_SPACE_VELOCITY_LINEAR_VALID_BIT | XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
    }
  }

  // Map hand parts to hand joints
  m_HandParts[WXRHandPart::Palm].PushBack(XR_HAND_JOINT_PALM_EXT);
  m_HandParts[WXRHandPart::Palm].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Wrist].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_TIP_EXT);
  m_HandParts[WXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_DISTAL_EXT);
  m_HandParts[WXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_PROXIMAL_EXT);
  m_HandParts[WXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_METACARPAL_EXT);
  m_HandParts[WXRHandPart::Thumb].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_TIP_EXT);
  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_DISTAL_EXT);
  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT);
  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_PROXIMAL_EXT);
  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_METACARPAL_EXT);
  m_HandParts[WXRHandPart::Index].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_TIP_EXT);
  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_DISTAL_EXT);
  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT);
  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT);
  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_METACARPAL_EXT);
  m_HandParts[WXRHandPart::Middle].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_TIP_EXT);
  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_DISTAL_EXT);
  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_INTERMEDIATE_EXT);
  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_PROXIMAL_EXT);
  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_METACARPAL_EXT);
  m_HandParts[WXRHandPart::Ring].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_TIP_EXT);
  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_DISTAL_EXT);
  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT);
  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_PROXIMAL_EXT);
  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_METACARPAL_EXT);
  m_HandParts[WXRHandPart::Little].PushBack(XR_HAND_JOINT_WRIST_EXT);
}

WOpenXRHandTracking::~WOpenXRHandTracking()
{
  for (WUInt32 uiSide : {0, 1})
  {
    XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrDestroyHandTrackerEXT(m_HandTracker[uiSide]));
  }
}

WXRHandTrackingInterface::HandPartTrackingState WOpenXRHandTracking::TryGetBoneTransforms(
  WEnum<WXRHand> hand, WEnum<WXRHandPart> handPart, WEnum<WXRTransformSpace> space, WDynamicArray<WXRHandBone>& out_bones)
{
  W_ASSERT_DEV(handPart <= WXRHandPart::Little, "Invalid hand part.");
  out_bones.Clear();

  for (WUInt32 uiJointIndex : m_HandParts[handPart])
  {
    const JointData& jointData = m_JointData[hand][uiJointIndex];
    if (!jointData.m_bValid)
      return WXRHandTrackingInterface::HandPartTrackingState::Untracked;

    out_bones.PushBack(jointData.m_Bone);
  }

  if (space == WXRTransformSpace::Global)
  {
    WWorld* pWorld = m_pOpenXR->GetWorld();
    if (!pWorld)
      return WXRHandTrackingInterface::HandPartTrackingState::NotSupported;

    if (const WStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<WStageSpaceComponentManager>())
    {
      if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        const WTransform globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
        for (WXRHandBone& bone : out_bones)
        {
          WTransform local = bone.m_Transform;
          bone.m_Transform = WTransform::MakeGlobalTransform(globalStageTransform, local);
        }
      }
    }
  }
  return WXRHandTrackingInterface::HandPartTrackingState::Tracked;
}

void WOpenXRHandTracking::UpdateJointTransforms()
{
  W_PROFILE_SCOPE("UpdateJointTransforms");
  const XrTime time = m_pOpenXR->m_FrameState.predictedDisplayTime;
  XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
  locateInfo.baseSpace = m_pOpenXR->GetBaseSpace();
  locateInfo.time = time;

  for (WUInt32 uiSide : {0, 1})
  {
    for (WUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
    {
      m_JointData[uiSide][i].m_Bone.m_Transform.SetIdentity();
      m_JointVelocities[uiSide][i].velocityFlags = XR_SPACE_VELOCITY_LINEAR_VALID_BIT | XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
    }
  }

  for (WUInt32 uiSide : {0, 1})
  {
    if (m_pOpenXR->m_Extensions.pfn_xrLocateHandJointsEXT(m_HandTracker[uiSide], &locateInfo, &m_Locations[uiSide]) != XrResult::XR_SUCCESS)
      m_Locations[uiSide].isActive = false;

    if (m_Locations[uiSide].isActive)
    {
      for (WUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        const XrHandJointLocationEXT& spaceLocation = m_JointLocations[uiSide][i];
        if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
        {
          m_JointData[uiSide][i].m_bValid = true;
          m_JointData[uiSide][i].m_Bone.m_fRadius = spaceLocation.radius;
          m_JointData[uiSide][i].m_Bone.m_Transform.m_vPosition = WOpenXRConversionUtils::ConvertPosition(spaceLocation.pose.position);
          m_JointData[uiSide][i].m_Bone.m_Transform.m_qRotation = WOpenXRConversionUtils::ConvertOrientation(spaceLocation.pose.orientation);
        }
        else
        {
          m_JointData[uiSide][i].m_bValid = false;
        }
      }
    }
    else
    {
      for (WUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        m_JointData[uiSide][i].m_bValid = false;
      }
    }
  }
}

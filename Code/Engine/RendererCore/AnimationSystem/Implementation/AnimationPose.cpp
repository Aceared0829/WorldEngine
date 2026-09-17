#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererFoundation/Shader/Types.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgAnimationPosePreparing);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgAnimationPosePreparing, 1, WRTTIDefaultAllocator<WMsgAnimationPosePreparing>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgInjectPoseCommands);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgInjectPoseCommands, 1, WRTTIDefaultAllocator<WMsgInjectPoseCommands>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgAnimationPoseUpdated);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgAnimationPoseUpdated, 1, WRTTIDefaultAllocator<WMsgAnimationPoseUpdated>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgRopePoseUpdated);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgRopePoseUpdated, 1, WRTTIDefaultAllocator<WMsgRopePoseUpdated>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgQueryAnimationSkeleton);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgQueryAnimationSkeleton, 1, WRTTIDefaultAllocator<WMsgQueryAnimationSkeleton>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgApplyRootMotion);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgApplyRootMotion, 1, WRTTIDefaultAllocator<WMsgApplyRootMotion>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Translation", m_vTranslation),
    W_MEMBER_PROPERTY("RotationX", m_RotationX),
    W_MEMBER_PROPERTY("RotationY", m_RotationY),
    W_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgAnimationCurveValue);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgAnimationCurveValue, 1, WRTTIDefaultAllocator<WMsgAnimationCurveValue>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CurveName", m_sCurveName),
    W_MEMBER_PROPERTY("Min", m_fMin),
    W_MEMBER_PROPERTY("Max", m_fMax),
    W_MEMBER_PROPERTY("Average", m_fAverage),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgRetrieveBoneState);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgRetrieveBoneState, 1, WRTTIDefaultAllocator<WMsgRetrieveBoneState>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WAnimationInvisibleUpdateRate, 1)
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::FullUpdate),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Max60FPS),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Max30FPS),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Max15FPS),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Max10FPS),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Max5FPS),
  W_ENUM_CONSTANT(WAnimationInvisibleUpdateRate::Pause),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WTime WAnimationInvisibleUpdateRate::GetTimeStep(WAnimationInvisibleUpdateRate::Enum value)
{
  switch (value)
  {
    case WAnimationInvisibleUpdateRate::FullUpdate:
      return WTime::MakeZero();
    case WAnimationInvisibleUpdateRate::Max60FPS:
      return WTime::MakeFromSeconds(1.0 / 60.0);
    case WAnimationInvisibleUpdateRate::Max30FPS:
      return WTime::MakeFromSeconds(1.0 / 30.0);
    case WAnimationInvisibleUpdateRate::Max15FPS:
      return WTime::MakeFromSeconds(1.0 / 15.0);
    case WAnimationInvisibleUpdateRate::Max10FPS:
      return WTime::MakeFromSeconds(1.0 / 10.0);

    case WAnimationInvisibleUpdateRate::Max5FPS:
    case WAnimationInvisibleUpdateRate::Pause: // full pausing should be handled separately, and if something isn't fully paused, it should behave like a very low update rate
      return WTime::MakeFromSeconds(1.0 / 5.0);

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return WTime::MakeZero();
}

void WMsgAnimationPoseUpdated::ComputeFullBoneTransform(WUInt32 uiJointIndex, WMat4& ref_mFullTransform) const
{
  ref_mFullTransform = m_pRootTransform->GetAsMat4() * m_ModelTransforms[uiJointIndex];
}

void WMsgAnimationPoseUpdated::ComputeFullBoneTransform(const WMat4& mRootTransform, const WMat4& mModelTransform, WMat4& ref_mFullTransform, WQuat& ref_qRotationOnly)
{
  ref_mFullTransform = mRootTransform * mModelTransform;

  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  ref_qRotationOnly.ReconstructFromMat4(ref_mFullTransform);
}

void WMsgAnimationPoseUpdated::ComputeFullBoneTransform(WUInt32 uiJointIndex, WMat4& ref_mFullTransform, WQuat& ref_qRotationOnly) const
{
  ComputeFullBoneTransform(m_pRootTransform->GetAsMat4(), m_ModelTransforms[uiJointIndex], ref_mFullTransform, ref_qRotationOnly);
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);

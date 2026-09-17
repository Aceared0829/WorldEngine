#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

/// The skeleton builder class provides the means to build skeleton instances from scratch.
/// This class is not necessary to use skeletons, usually they should be deserialized from data created by the tools.
class W_RENDERERCORE_DLL WSkeletonBuilder
{

public:
  WSkeletonBuilder();
  ~WSkeletonBuilder();

  /// Adds a joint to the skeleton
  /// Since the only way to add a joint with a parent is through this method the order of joints in the array is guaranteed
  /// so that child joints always come after their parent joints
  WUInt16 AddJoint(WStringView sName, const WTransform& localRestPose, WUInt16 uiParentIndex = WInvalidJointIndex);

  void SetJointLimit(WUInt16 uiJointIndex, const WQuat& qLocalOrientation, WSkeletonJointType::Enum jointType, WAngle halfSwingLimitY, WAngle halfSwingLimitZ, WAngle twistLimitHalfAngle, WAngle twistLimitCenterAngle, float fStiffness);

  void SetJointSurface(WUInt16 uiJointIndex, WStringView sSurface);
  void SetJointCollisionLayer(WUInt16 uiJointIndex, WUInt8 uiCollsionLayer);

  /// Creates a skeleton from the accumulated data.
  void BuildSkeleton(WSkeleton& ref_skeleton) const;

  /// Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    WTransform m_RestPoseLocal;
    WTransform m_RestPoseGlobal; // this one is temporary and not stored in the final WSkeleton
    WTransform m_InverseRestPoseGlobal;
    WUInt16 m_uiParentIndex = WInvalidJointIndex;
    WHashedString m_sName;
    WEnum<WSkeletonJointType> m_JointType;
    WQuat m_qLocalJointOrientation = WQuat::MakeIdentity();
    WAngle m_HalfSwingLimitZ;
    WAngle m_HalfSwingLimitY;
    WAngle m_TwistLimitHalfAngle;
    WAngle m_TwistLimitCenterAngle;
    float m_fStiffness = 0.0f;

    WString m_sSurface;
    WUInt8 m_uiCollisionLayer = 0;
  };

  WDeque<BuilderJoint> m_Joints;
};

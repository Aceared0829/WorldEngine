#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class WStreamWriter;
class WStreamReader;
class WSkeletonBuilder;
class WSkeleton;

using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

namespace ozz::animation
{
  class Skeleton;
}

/// Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class W_RENDERERCORE_DLL WSkeletonJoint
{
public:
  const WTransform& GetRestPoseLocalTransform() const { return m_RestPoseLocal; }

  /// Returns WInvalidJointIndex if no parent
  WUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool IsRootJoint() const { return m_uiParentIndex == WInvalidJointIndex; }
  const WHashedString& GetName() const { return m_sName; }

  WAngle GetHalfSwingLimitY() const { return m_HalfSwingLimitY; }
  WAngle GetHalfSwingLimitZ() const { return m_HalfSwingLimitZ; }
  WAngle GetTwistLimitHalfAngle() const { return m_TwistLimitHalfAngle; }
  WAngle GetTwistLimitCenterAngle() const { return m_TwistLimitCenterAngle; }
  WAngle GetTwistLimitLow() const;
  WAngle GetTwistLimitHigh() const;
  WEnum<WSkeletonJointType> GetJointType() const { return m_JointType; }

  WQuat GetLocalOrientation() const { return m_qLocalJointOrientation; }

  WSurfaceResourceHandle GetSurface() const { return m_hSurface; }
  WUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }

  float GetStiffness() const { return m_fStiffness; }
  void SetStiffness(float fValue) { m_fStiffness = fValue; }

private:
  friend WSkeleton;
  friend WSkeletonBuilder;

  WTransform m_RestPoseLocal;
  WUInt16 m_uiParentIndex = WInvalidJointIndex;
  WHashedString m_sName;

  WSurfaceResourceHandle m_hSurface;
  WUInt8 m_uiCollisionLayer = 0;

  WEnum<WSkeletonJointType> m_JointType;
  WQuat m_qLocalJointOrientation = WQuat::MakeIdentity();
  WAngle m_HalfSwingLimitY;
  WAngle m_HalfSwingLimitZ;
  WAngle m_TwistLimitHalfAngle;
  WAngle m_TwistLimitCenterAngle;
  float m_fStiffness = 0.0f;
};

/// The skeleton class encapsulates the information about the joint structure for a model.
class W_RENDERERCORE_DLL WSkeleton
{
  W_DISALLOW_COPY_AND_ASSIGN(WSkeleton);

public:
  WSkeleton();
  WSkeleton(WSkeleton&& rhs);
  ~WSkeleton();

  void operator=(WSkeleton&& rhs);

  /// Returns the number of joints in the skeleton.
  WUInt16 GetJointCount() const { return static_cast<WUInt16>(m_Joints.GetCount()); }

  /// Returns the nth joint.
  const WSkeletonJoint& GetJointByIndex(WUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// Allows to find a specific joint in the skeleton by name. Returns WInvalidJointIndex if not found
  WUInt16 FindJointByName(const WTempHashedString& sName) const;

  /// Checks if two skeletons are compatible (same joint count and hierarchy)
  // bool IsCompatibleWith(const WSkeleton& other) const;

  /// Saves the skeleton in a given stream.
  void Save(WStreamWriter& inout_stream) const;

  /// Loads the skeleton from the given stream.
  void Load(WStreamReader& inout_stream);

  bool IsJointDescendantOf(WUInt16 uiJoint, WUInt16 uiExpectedParent) const;

  const ozz::animation::Skeleton& GetOzzSkeleton() const;

  WUInt64 GetHeapMemoryUsage() const;

  /// The direction in which the bones shall point for visualization
  WEnum<WBasisAxis> m_BoneDirection;

protected:
  friend WSkeletonBuilder;

  WDynamicArray<WSkeletonJoint> m_Joints;
  mutable WUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
};

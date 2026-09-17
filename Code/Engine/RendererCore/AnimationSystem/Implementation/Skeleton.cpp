#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSkeletonJointType, 1)
  W_ENUM_CONSTANTS(WSkeletonJointType::None, WSkeletonJointType::Fixed, WSkeletonJointType::SwingTwist)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WSkeleton::WSkeleton() = default;
WSkeleton::~WSkeleton() = default;

WSkeleton::WSkeleton(WSkeleton&& rhs)
{
  *this = std::move(rhs);
}

void WSkeleton::operator=(WSkeleton&& rhs)
{
  m_Joints = std::move(rhs.m_Joints);
  m_pOzzSkeleton = std::move(rhs.m_pOzzSkeleton);
}

WUInt16 WSkeleton::FindJointByName(const WTempHashedString& sJointName) const
{
  const WUInt16 uiJointCount = static_cast<WUInt16>(m_Joints.GetCount());
  for (WUInt16 i = 0; i < uiJointCount; ++i)
  {
    if (m_Joints[i].GetName() == sJointName)
    {
      return i;
    }
  }

  return WInvalidJointIndex;
}

void WSkeleton::Save(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(7);

  const WUInt32 uiNumJoints = m_Joints.GetCount();
  inout_stream << uiNumJoints;

  for (WUInt32 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream << m_Joints[i].m_sName;
    inout_stream << m_Joints[i].m_uiParentIndex;
    inout_stream << m_Joints[i].m_RestPoseLocal;

    inout_stream << m_Joints[i].m_qLocalJointOrientation;
    inout_stream << m_Joints[i].m_HalfSwingLimitZ;
    inout_stream << m_Joints[i].m_HalfSwingLimitY;
    inout_stream << m_Joints[i].m_TwistLimitHalfAngle;
    inout_stream << m_Joints[i].m_TwistLimitCenterAngle;

    inout_stream << m_Joints[i].m_JointType;
    inout_stream << m_Joints[i].m_hSurface;
    inout_stream << m_Joints[i].m_uiCollisionLayer;
    inout_stream << m_Joints[i].m_fStiffness;
  }

  inout_stream << m_BoneDirection;
}

void WSkeleton::Load(WStreamReader& inout_stream)
{
  const WTypeVersion version = inout_stream.ReadVersion(7);
  if (version < 3)
    return;

  m_Joints.Clear();

  WUInt32 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_Joints.Reserve(uiNumJoints);

  for (WUInt32 i = 0; i < uiNumJoints; ++i)
  {
    WSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

    inout_stream >> joint.m_sName;
    inout_stream >> joint.m_uiParentIndex;
    inout_stream >> joint.m_RestPoseLocal;

    if (version >= 5)
    {
      inout_stream >> m_Joints[i].m_qLocalJointOrientation;
      inout_stream >> m_Joints[i].m_HalfSwingLimitZ;
      inout_stream >> m_Joints[i].m_HalfSwingLimitY;
      inout_stream >> m_Joints[i].m_TwistLimitHalfAngle;
      inout_stream >> m_Joints[i].m_TwistLimitCenterAngle;
    }

    if (version >= 6)
    {
      inout_stream >> m_Joints[i].m_JointType;
      inout_stream >> m_Joints[i].m_hSurface;
      inout_stream >> m_Joints[i].m_uiCollisionLayer;
    }

    if (version >= 7)
    {
      inout_stream >> m_Joints[i].m_fStiffness;
    }
  }

  if (version >= 4)
  {
    inout_stream >> m_BoneDirection;
  }
}

bool WSkeleton::IsJointDescendantOf(WUInt16 uiJoint, WUInt16 uiExpectedParent) const
{
  if (uiExpectedParent == WInvalidJointIndex)
    return true;

  while (uiJoint != WInvalidJointIndex)
  {
    if (uiJoint == uiExpectedParent)
      return true;

    uiJoint = m_Joints[uiJoint].m_uiParentIndex;
  }

  return false;
}

static void BuildRawOzzSkeleton(const WSkeleton& skeleton, WUInt16 uiExpectedParent, ozz::animation::offline::RawSkeleton::Joint::Children& ref_dstBones)
{
  WTempHybridArray<WUInt16, 6> children;

  for (WUInt16 i = 0; i < skeleton.GetJointCount(); ++i)
  {
    if (skeleton.GetJointByIndex(i).GetParentIndex() == uiExpectedParent)
    {
      children.PushBack(i);
    }
  }

  ref_dstBones.resize((size_t)children.GetCount());

  for (WUInt16 i = 0; i < children.GetCount(); ++i)
  {
    const auto& srcJoint = skeleton.GetJointByIndex(children[i]);
    const auto& srcTransform = srcJoint.GetRestPoseLocalTransform();
    auto& dstJoint = ref_dstBones[i];

    dstJoint.name = srcJoint.GetName().GetData();

    dstJoint.transform.translation.x = srcTransform.m_vPosition.x;
    dstJoint.transform.translation.y = srcTransform.m_vPosition.y;
    dstJoint.transform.translation.z = srcTransform.m_vPosition.z;
    dstJoint.transform.rotation.x = srcTransform.m_qRotation.x;
    dstJoint.transform.rotation.y = srcTransform.m_qRotation.y;
    dstJoint.transform.rotation.z = srcTransform.m_qRotation.z;
    dstJoint.transform.rotation.w = srcTransform.m_qRotation.w;
    dstJoint.transform.scale.x = srcTransform.m_vScale.x;
    dstJoint.transform.scale.y = srcTransform.m_vScale.y;
    dstJoint.transform.scale.z = srcTransform.m_vScale.z;

    BuildRawOzzSkeleton(skeleton, children[i], dstJoint.children);
  }
}

const ozz::animation::Skeleton& WSkeleton::GetOzzSkeleton() const
{
  if (m_pOzzSkeleton)
    return *m_pOzzSkeleton.Borrow();

  // caching the skeleton isn't thread-safe
  static WMutex cacheSkeletonMutex;
  W_LOCK(cacheSkeletonMutex);

  // skip this, if the skeleton has been created in the mean-time
  if (m_pOzzSkeleton == nullptr)
  {
    ozz::animation::offline::RawSkeleton rawSkeleton;
    BuildRawOzzSkeleton(*this, WInvalidJointIndex, rawSkeleton.roots);

    ozz::animation::offline::SkeletonBuilder skeletonBuilder;
    const auto pOzzSkeleton = skeletonBuilder(rawSkeleton);

    auto ozzSkeleton = W_DEFAULT_NEW(ozz::animation::Skeleton);

    WOzzUtils::CopySkeleton(ozzSkeleton, pOzzSkeleton.get());

    // since the pointer is read outside the mutex, only assign it, once it is fully ready for use
    m_pOzzSkeleton = ozzSkeleton;
  }

  return *m_pOzzSkeleton.Borrow();
}

WUInt64 WSkeleton::GetHeapMemoryUsage() const
{
  return m_Joints.GetHeapMemoryUsage(); // TODO: + ozz skeleton
}

WAngle WSkeletonJoint::GetTwistLimitLow() const
{
  WAngle base = m_TwistLimitCenterAngle;
  base.NormalizeRange();

  if (base > WAngle::MakeFromDegree(180))
    base -= WAngle::MakeFromDegree(360);

  return WMath::Max(WAngle::MakeFromDegree(-179), base - m_TwistLimitHalfAngle);
}

WAngle WSkeletonJoint::GetTwistLimitHigh() const
{
  WAngle base = m_TwistLimitCenterAngle;
  base.NormalizeRange();

  if (base > WAngle::MakeFromDegree(180))
    base -= WAngle::MakeFromDegree(360);

  return WMath::Min(WAngle::MakeFromDegree(179), base + m_TwistLimitHalfAngle);
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_Skeleton);

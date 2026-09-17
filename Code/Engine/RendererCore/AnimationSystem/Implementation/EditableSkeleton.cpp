#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSkeletonJointGeometryType, 1)
W_ENUM_CONSTANTS(WSkeletonJointGeometryType::None, WSkeletonJointGeometryType::Capsule, WSkeletonJointGeometryType::CapsuleSideways, WSkeletonJointGeometryType::Sphere, WSkeletonJointGeometryType::Box)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditableSkeletonBoneShape, 1, WRTTIDefaultAllocator<WEditableSkeletonBoneShape>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Geometry", WSkeletonJointGeometryType, m_Geometry),
    W_MEMBER_PROPERTY("Offset", m_vOffset),
    W_MEMBER_PROPERTY("Rotation", m_qRotation),
    W_MEMBER_PROPERTY("Length", m_fLength)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.01f, 10.0f)),
    W_MEMBER_PROPERTY("Width", m_fWidth)->AddAttributes(new WDefaultValueAttribute(0.05f), new WClampValueAttribute(0.01f, 10.0f)),
    W_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new WDefaultValueAttribute(0.05f), new WClampValueAttribute(0.01f, 10.0f)),

  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditableSkeletonBoneCollider, 1, WRTTIDefaultAllocator<WEditableSkeletonBoneCollider>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Identifier", m_sIdentifier)->AddAttributes(new WHiddenAttribute()),
    W_ARRAY_MEMBER_PROPERTY("VertexPositions", m_VertexPositions)->AddAttributes(new WHiddenAttribute()),
    W_ARRAY_MEMBER_PROPERTY("TriangleIndices", m_TriangleIndices)->AddAttributes(new WHiddenAttribute()),

  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditableSkeletonJoint, 2, WRTTIDefaultAllocator<WEditableSkeletonJoint>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Name", GetName, SetName)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(WPropertyFlags::Hidden)->AddAttributes(new WDefaultValueAttribute(WTransform::MakeIdentity())),
    W_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    W_ENUM_MEMBER_PROPERTY("JointType", WSkeletonJointType, m_JointType),
    W_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new WDefaultValueAttribute(10.0f)),
    W_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(170)), new WDefaultValueAttribute(WAngle::MakeFromDegree(30))),
    W_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(170)), new WDefaultValueAttribute(WAngle::MakeFromDegree(30))),
    W_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(10), WAngle::MakeFromDegree(170)), new WDefaultValueAttribute(WAngle::MakeFromDegree(30))),
    W_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle),
    W_MEMBER_PROPERTY("OverrideSurface", m_bOverrideSurface),
    W_MEMBER_PROPERTY("Surface", m_sSurfaceOverride)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("OverrideCollisionLayer", m_bOverrideCollisionLayer),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayerOverride)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),

    W_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(WPropertyFlags::PointerOwner | WPropertyFlags::Hidden),
    W_ARRAY_MEMBER_PROPERTY("BoneShapes", m_BoneShapes),
    W_ARRAY_MEMBER_PROPERTY("Colliders", m_BoneColliders)->AddAttributes(new WContainerAttribute(false, false, false)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTransformManipulatorAttribute(nullptr, "LocalRotation", nullptr, "GizmoOffsetTranslationRO", "GizmoOffsetRotationRO"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditableSkeleton, 2, WRTTIDefaultAllocator<WEditableSkeleton>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new WFileBrowserAttribute("Select Mesh", WFileBrowserAttribute::MeshesWithAnimations), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("ImportTransform", WMeshImportTransform, m_ImportTransform),
    W_ENUM_MEMBER_PROPERTY("RightDir", WBasisAxis, m_RightDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::NegativeX)),
    W_ENUM_MEMBER_PROPERTY("UpDir", WBasisAxis, m_UpDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::PositiveY)),
    W_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    W_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0001f, 10000.0f)),
    W_ENUM_MEMBER_PROPERTY("BoneDirection", WBasisAxis, m_BoneDirection)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::PositiveY)),
    W_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", WDependencyFlags::None)),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("MaxImpulse", m_fMaxImpulse)->AddAttributes(new WDefaultValueAttribute(100.f)),
    W_MEMBER_PROPERTY("LeftFootJoint", m_sLeftFootJoint),
    W_MEMBER_PROPERTY("RightFootJoint", m_sRightFootJoint),

    W_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(WPropertyFlags::PointerOwner | WPropertyFlags::Hidden),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WExposedBone, WNoBase, 1, WRTTIDefaultAllocator<WExposedBone>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Parent", m_sParent),
    W_MEMBER_PROPERTY("Transform", m_Transform),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_DEFINE_CUSTOM_VARIANT_TYPE(WExposedBone);
// clang-format on


void operator<<(WStreamWriter& inout_stream, const WExposedBone& bone)
{
  inout_stream << bone.m_sName;
  inout_stream << bone.m_sParent;
  inout_stream << bone.m_Transform;
}

void operator>>(WStreamReader& inout_stream, WExposedBone& ref_bone)
{
  inout_stream >> ref_bone.m_sName;
  inout_stream >> ref_bone.m_sParent;
  inout_stream >> ref_bone.m_Transform;
}

bool operator==(const WExposedBone& lhs, const WExposedBone& rhs)
{
  if (lhs.m_sName != rhs.m_sName)
    return false;
  if (lhs.m_sParent != rhs.m_sParent)
    return false;
  if (lhs.m_Transform != rhs.m_Transform)
    return false;
  return true;
}

WEditableSkeleton::WEditableSkeleton() = default;
WEditableSkeleton::~WEditableSkeleton()
{
  ClearJoints();
}

void WEditableSkeleton::ClearJoints()
{
  for (WEditableSkeletonJoint* pChild : m_Children)
  {
    W_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void WEditableSkeleton::CreateJointsRecursive(WSkeletonBuilder& ref_sb, WSkeletonResourceDescriptor& ref_desc, const WEditableSkeletonJoint* pParentJoint, const WEditableSkeletonJoint* pThisJoint, WUInt16 uiThisJointIdx, const WQuat& qParentAccuRot, const WMat4& mRootTransform) const
{
  for (auto& shape : pThisJoint->m_BoneShapes)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();

    geo.m_Type = shape.m_Geometry;
    geo.m_uiAttachedToJoint = static_cast<WUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(shape.m_fLength, shape.m_fWidth, shape.m_fThickness);
    geo.m_Transform.m_vPosition = shape.m_vOffset;
    geo.m_Transform.m_qRotation = shape.m_qRotation;
  }

  for (auto& shape : pThisJoint->m_BoneColliders)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();
    geo.m_Type = WSkeletonJointGeometryType::ConvexMesh;
    geo.m_uiAttachedToJoint = static_cast<WUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_VertexPositions = shape.m_VertexPositions;
    geo.m_TriangleIndices = shape.m_TriangleIndices;
  }

  const WVec3 s = pThisJoint->m_LocalTransform.m_vScale;
  if (!s.IsEqual(WVec3(1), 0.1f))
  {
    // WLog::Warning("Mesh bone '{}' has scaling values of {}/{}/{} - this is not supported.", pThisJoint->m_sName, s.x, s.y, s.z);
  }

  const WQuat qThisAccuRot = qParentAccuRot * pThisJoint->m_LocalTransform.m_qRotation;
  WQuat qParentGlobalRot;

  {
    // as always, the root transform is the bane of my existence
    // since it can contain mirroring, the final global rotation of a joint will be incorrect if we don't incorporate the root scale
    // unfortunately this can't be done once for the first node, but has to be done on the result instead

    WMat4 full;
    WMsgAnimationPoseUpdated::ComputeFullBoneTransform(mRootTransform, qParentAccuRot.GetAsMat4(), full, qParentGlobalRot);
  }

  ref_sb.SetJointLimit(uiThisJointIdx, pThisJoint->m_qLocalJointRotation, pThisJoint->m_JointType, pThisJoint->m_SwingLimitY, pThisJoint->m_SwingLimitZ, pThisJoint->m_TwistLimitHalfAngle, pThisJoint->m_TwistLimitCenterAngle, pThisJoint->m_fStiffness);

  ref_sb.SetJointCollisionLayer(uiThisJointIdx, pThisJoint->m_bOverrideCollisionLayer ? pThisJoint->m_uiCollisionLayerOverride : m_uiCollisionLayer);
  ref_sb.SetJointSurface(uiThisJointIdx, pThisJoint->m_bOverrideSurface ? pThisJoint->m_sSurfaceOverride : m_sSurfaceFile);

  for (const auto* pChildJoint : pThisJoint->m_Children)
  {
    const WUInt16 uiChildJointIdx = ref_sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_LocalTransform, uiThisJointIdx);

    CreateJointsRecursive(ref_sb, ref_desc, pThisJoint, pChildJoint, uiChildJointIdx, qThisAccuRot, mRootTransform);
  }
}

void WEditableSkeleton::FillResourceDescriptor(WSkeletonResourceDescriptor& ref_desc) const
{
  ref_desc.m_fMaxImpulse = m_fMaxImpulse;
  ref_desc.m_Geometry.Clear();

  WSkeletonBuilder sb;
  for (const auto* pJoint : m_Children)
  {
    const WUInt16 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_LocalTransform);

    CreateJointsRecursive(sb, ref_desc, nullptr, pJoint, idx, WQuat::MakeIdentity(), ref_desc.m_RootTransform.GetAsMat4());
  }

  sb.BuildSkeleton(ref_desc.m_Skeleton);
  ref_desc.m_Skeleton.m_BoneDirection = m_BoneDirection;

  ref_desc.m_uiLeftFootJoint = ref_desc.m_Skeleton.FindJointByName(WTempHashedString(m_sLeftFootJoint));
  ref_desc.m_uiRightFootJoint = ref_desc.m_Skeleton.FindJointByName(WTempHashedString(m_sRightFootJoint));
}

static void BuildOzzRawSkeleton(const WEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& ref_dstJoint)
{
  ref_dstJoint.name = srcJoint.m_sName.GetString();
  ref_dstJoint.transform.translation.x = srcJoint.m_LocalTransform.m_vPosition.x;
  ref_dstJoint.transform.translation.y = srcJoint.m_LocalTransform.m_vPosition.y;
  ref_dstJoint.transform.translation.z = srcJoint.m_LocalTransform.m_vPosition.z;
  ref_dstJoint.transform.rotation.x = srcJoint.m_LocalTransform.m_qRotation.x;
  ref_dstJoint.transform.rotation.y = srcJoint.m_LocalTransform.m_qRotation.y;
  ref_dstJoint.transform.rotation.z = srcJoint.m_LocalTransform.m_qRotation.z;
  ref_dstJoint.transform.rotation.w = srcJoint.m_LocalTransform.m_qRotation.w;
  ref_dstJoint.transform.scale.x = srcJoint.m_LocalTransform.m_vScale.x;
  ref_dstJoint.transform.scale.y = srcJoint.m_LocalTransform.m_vScale.y;
  ref_dstJoint.transform.scale.z = srcJoint.m_LocalTransform.m_vScale.z;

  ref_dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (WUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], ref_dstJoint.children[b]);
  }
}

void WEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const
{
  out_skeleton.roots.resize((size_t)m_Children.GetCount());

  for (WUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_skeleton.roots[b]);
  }
}

void WEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  WOzzUtils::CopySkeleton(&out_skeleton, pNewOzzSkeleton.get());
}

WEditableSkeletonJoint::WEditableSkeletonJoint() = default;

WEditableSkeletonJoint::~WEditableSkeletonJoint()
{
  ClearJoints();
}

const char* WEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void WEditableSkeletonJoint::SetName(const char* szSz)
{
  m_sName.Assign(szSz);
}

void WEditableSkeletonJoint::ClearJoints()
{
  for (WEditableSkeletonJoint* pChild : m_Children)
  {
    W_DEFAULT_DELETE(pChild);
  }
  m_Children.Clear();
}

void WEditableSkeletonJoint::CopyPropertiesFrom(const WEditableSkeletonJoint* pJoint)
{
  // copy existing (user edited) properties from pJoint into this joint
  // which has just been imported from file

  // do not copy:
  //  name
  //  transform
  //  children
  //  bone collider geometry (vertices, indices)

  // synchronize user config of bone colliders
  for (WUInt32 i = 0; i < m_BoneColliders.GetCount(); ++i)
  {
    auto& dst = m_BoneColliders[i];

    for (WUInt32 j = 0; j < pJoint->m_BoneColliders.GetCount(); ++j)
    {
      const auto& src = pJoint->m_BoneColliders[j];

      if (dst.m_sIdentifier == src.m_sIdentifier)
      {
        // dst.m_bOverrideSurface = src.m_bOverrideSurface;
        // dst.m_bOverrideCollisionLayer = src.m_bOverrideCollisionLayer;
        // dst.m_sSurfaceOverride = src.m_sSurfaceOverride;
        // dst.m_uiCollisionLayerOverride = src.m_uiCollisionLayerOverride;
        break;
      }
    }
  }

  m_BoneShapes = pJoint->m_BoneShapes;
  m_qLocalJointRotation = pJoint->m_qLocalJointRotation;
  m_JointType = pJoint->m_JointType;
  m_SwingLimitY = pJoint->m_SwingLimitY;
  m_SwingLimitZ = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle = pJoint->m_TwistLimitCenterAngle;
  m_fStiffness = pJoint->m_fStiffness;

  m_bOverrideSurface = pJoint->m_bOverrideSurface;
  m_bOverrideCollisionLayer = pJoint->m_bOverrideCollisionLayer;
  m_sSurfaceOverride = pJoint->m_sSurfaceOverride;
  m_uiCollisionLayerOverride = pJoint->m_uiCollisionLayerOverride;
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);

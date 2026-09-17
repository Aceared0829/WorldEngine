#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSkeletonComponent, 5, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    W_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeBones)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    W_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
    W_MEMBER_PROPERTY("VisualizeSwingLimits", m_bVisualizeSwingLimits),
    W_MEMBER_PROPERTY("VisualizeTwistLimits", m_bVisualizeTwistLimits),
    W_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    W_MESSAGE_HANDLER(WMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSkeletonComponent::WSkeletonComponent() = default;
WSkeletonComponent::~WSkeletonComponent() = default;

WResult WSkeletonComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_MaxBounds.IsValid())
  {
    WBoundingBox bbox = m_MaxBounds;
    ref_bounds = WBoundingBoxSphere::MakeFromBox(bbox);
    ref_bounds.Transform(m_RootTransform.GetAsMat4());
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WSkeletonComponent::Update()
{
  if (m_hSkeleton.IsValid() && (m_bVisualizeBones || m_bVisualizeColliders || m_bVisualizeJoints || m_bVisualizeSwingLimits || m_bVisualizeTwistLimits))
  {
    WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
    {
      VisualizeSkeletonDefaultState();
    }

    const WQuat qBoneDir = WBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
    const WVec3 vBoneDir = qBoneDir * WVec3(1, 0, 0);
    const WVec3 vBoneTangent = qBoneDir * WVec3(0, 1, 0);

    WDebugRenderer::DrawLinesOccluded(GetWorld(), m_LinesSkeleton, WColor::White, GetOwner()->GetGlobalTransform());
    WDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, WColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresShapes)
    {
      WDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxShapes)
    {
      WDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsuleShapes)
    {
      WDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_AngleShapes)
    {
      WDebugRenderer::DrawAngle(GetWorld(), shape.m_StartAngle, shape.m_EndAngle, WColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform, vBoneTangent, vBoneDir);
    }

    for (const auto& shape : m_ConeLimitShapes)
    {
      WDebugRenderer::DrawLimitCone(GetWorld(), shape.m_Angle1, shape.m_Angle2, WColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CylinderShapes)
    {
      WDebugRenderer::DrawCylinder(GetWorld(), shape.m_fRadius1, shape.m_fRadius2, shape.m_fLength, shape.m_Color, WColor::MakeZero(), GetOwner()->GetGlobalTransform() * shape.m_Transform, false, false);
    }
  }
}

void WSkeletonComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeBones;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
  s << m_bVisualizeSwingLimits;
  s << m_bVisualizeTwistLimits;
}

void WSkeletonComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion <= 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeBones;
  s >> m_sBonesToHighlight;
  s >> m_bVisualizeColliders;
  s >> m_bVisualizeJoints;
  s >> m_bVisualizeSwingLimits;
  s >> m_bVisualizeTwistLimits;
}

void WSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds = WBoundingBox::MakeInvalid();
  VisualizeSkeletonDefaultState();
}

void WSkeletonComponent::SetSkeleton(const WSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds = WBoundingBox::MakeInvalid();
    VisualizeSkeletonDefaultState();
  }
}

void WSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    VisualizeSkeletonDefaultState();
  }
}

const char* WSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void WSkeletonComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresShapes.Clear();
  m_BoxShapes.Clear();
  m_CapsuleShapes.Clear();
  m_AngleShapes.Clear();
  m_ConeLimitShapes.Clear();
  m_CylinderShapes.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  BuildSkeletonVisualization(msg);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  WBoundingBox poseBounds;
  poseBounds = WBoundingBox::MakeInvalid();

  for (const auto& bone : msg.m_ModelTransforms)
  {
    poseBounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    QueueLocalBoundsUpdate();
  }
  else if (((WRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (W_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    QueueLocalBoundsUpdate();
  }
}

void WSkeletonComponent::BuildSkeletonVisualization(WMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeBones || !msg.m_pSkeleton)
    return;

  WStringBuilder tmp;

  struct Bone
  {
    WVec3 pos = WVec3::MakeZero();
    WVec3 dir = WVec3::MakeZero();
    float distToParent = 0.0f;
    float minDistToChild = 10.0f;
    bool highlight = false;
  };

  WTempHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const WVec3 vBoneDir = WBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int iCurrentBone, int iParentBone)
  {
    if (iParentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const WVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[iParentBone].GetTranslationVector();
    const WVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].GetTranslationVector();

    WVec3 dirToBone = (v1 - v0);

    auto& bone = bones[iCurrentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();

    auto& pb = bones[iParentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(WVec3::MakeZero()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < WAngle::MakeFromDegree(45))
      {
        WPlane plane;
        plane = WPlane::MakeFromNormalAndPoint(pb.dir, pb.pos);
        pb.minDistToChild = WMath::Min(pb.minDistToChild, plane.GetDistanceTo(v1));
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

  if (m_sBonesToHighlight == "*")
  {
    for (WUInt32 b = 0; b < bones.GetCount(); ++b)
    {
      bones[b].highlight = true;
    }
  }
  else if (!m_sBonesToHighlight.IsEmpty())
  {
    const WStringBuilder mask(";", m_sBonesToHighlight, ";");

    for (WUInt16 b = 0; b < static_cast<WUInt16>(bones.GetCount()); ++b)
    {
      const WString currentName = msg.m_pSkeleton->GetJointByIndex(b).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (mask.FindSubString(tmp))
      {
        bones[b].highlight = true;
      }
    }
  }

  for (WUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (!bone.highlight)
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = WMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      WVec3 v0 = bone.pos;
      WVec3 v1 = bone.pos + bone.dir * len;

      m_LinesSkeleton.PushBack(WDebugRendererLine(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = WColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = WColor::DarkCyan;
    }
  }

  for (WUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (bone.highlight && !bone.dir.IsZero(0.0001f))
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = WMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      WVec3 v0 = bone.pos;
      WVec3 v1 = bone.pos + bone.dir * len;

      const WVec3 vO1 = bone.dir.GetOrthogonalVector().GetNormalized();
      const WVec3 vO2 = bone.dir.CrossRH(vO1).GetNormalized();

      WVec3 s[4];
      s[0] = v0 + vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[1] = v0 + vO2 * len * 0.1f + bone.dir * len * 0.1f;
      s[2] = v0 - vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[3] = v0 - vO2 * len * 0.1f + bone.dir * len * 0.1f;

      m_LinesSkeleton.PushBack(WDebugRendererLine(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = WColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = WColor::DarkCyan;

      for (WUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(WDebugRendererLine(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = WColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = WColor::Chartreuse;

        m_LinesSkeleton.PushBack(WDebugRendererLine(s[si], v1));
        m_LinesSkeleton.PeekBack().m_startColor = WColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = WColor::Chartreuse;
      }
    }
  }
}

void WSkeletonComponent::BuildColliderVisualization(WMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeColliders || !msg.m_pSkeleton || !m_hSkeleton.IsValid())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const WQuat qBoneDirAdjustment = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveX, srcBoneDir);

  WStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  WStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  // the capsule should extend along X, but the debug renderer draws them along Z
  const WQuat qRotZtoX = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(-90));
  const WQuat qRotYtoZ = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90));

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == WSkeletonJointGeometryType::None)
      continue;

    WMat4 boneTrans;
    WQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const WColor hlS = WMath::Lerp(WColor::DimGrey, WColor::Yellow, bHighlight ? 1.0f : 0.2f);

    const WQuat qFinalBoneRot = boneRot * qBoneDirAdjustment;

    WTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.GetTranslationVector() + qFinalBoneRot * geo.m_Transform.m_vPosition;
    st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == WSkeletonJointGeometryType::Sphere)
    {
      auto& shape = m_SpheresShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Shape = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), geo.m_Transform.m_vScale.z);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == WSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxShapes.ExpandAndGetRef();

      WVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x * 0.5f;
      ext.y = geo.m_Transform.m_vScale.y * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * WVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      shape.m_Transform = st;
      shape.m_Shape = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), ext);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == WSkeletonJointGeometryType::Capsule)
    {
      st.m_qRotation = st.m_qRotation * qRotZtoX;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * WVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }

    if (geo.m_Type == WSkeletonJointGeometryType::CapsuleSideways)
    {
      st.m_qRotation = st.m_qRotation * qRotYtoZ;

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }

    if (geo.m_Type == WSkeletonJointGeometryType::ConvexMesh)
    {
      st.SetIdentity();
      st = *msg.m_pRootTransform;

      for (WUInt32 f = 0; f < geo.m_TriangleIndices.GetCount(); f += 3)
      {
        const WUInt32 i0 = geo.m_TriangleIndices[f + 0];
        const WUInt32 i1 = geo.m_TriangleIndices[f + 1];
        const WUInt32 i2 = geo.m_TriangleIndices[f + 2];

        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i0];
          l.m_end = st * geo.m_VertexPositions[i1];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i1];
          l.m_end = st * geo.m_VertexPositions[i2];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i2];
          l.m_end = st * geo.m_VertexPositions[i0];
        }
      }
    }
  }
}

void WSkeletonComponent::BuildJointVisualization(WMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || (!m_bVisualizeJoints && !m_bVisualizeSwingLimits && !m_bVisualizeTwistLimits))
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  WStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  WStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  const WQuat qBoneDir = WBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const WQuat qBoneDirT = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const WQuat qBoneDirBT = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const WQuat qBoneDirT2 = WBasisAxis::GetBasisRotation(WBasisAxis::NegativeY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);

  for (WUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto& thisJoint = skel.GetJointByIndex(uiJointIdx);
    const WUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (thisJoint.IsRootJoint())
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    WMat4 parentTrans;
    WQuat parentRot; // contains root transform
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    WMat4 thisTrans; // contains root transform
    WQuat thisRot;   // contains root transform
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans, thisRot);

    const WVec3 vJointPos = thisTrans.GetTranslationVector();
    const WQuat qLimitRot = parentRot * thisJoint.GetLocalOrientation();

    // main directions
    if (m_bVisualizeJoints && thisJoint.GetJointType() != WSkeletonJointType::None)
    {
      const WColor hlM = WMath::Lerp(WColor::OrangeRed, WColor::DimGrey, bHighlight ? 0 : 0.8f);
      const WColor hlT = WMath::Lerp(WColor::LawnGreen, WColor::DimGrey, bHighlight ? 0 : 0.8f);
      const WColor hlBT = WMath::Lerp(WColor::BlueViolet, WColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlBT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirBT;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // swing limit
    if (m_bVisualizeSwingLimits && thisJoint.GetJointType() == WSkeletonJointType::SwingTwist)
    {
      auto& shape = m_ConeLimitShapes.ExpandAndGetRef();
      shape.m_Angle1 = thisJoint.GetHalfSwingLimitY();
      shape.m_Angle2 = thisJoint.GetHalfSwingLimitZ();
      shape.m_Color = WMath::Lerp(WColor::DimGrey, WColor::DeepPink, bHighlight ? 1.0f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.05f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot * qBoneDir;

      const WColor hlM = WMath::Lerp(WColor::OrangeRed, WColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // twist limit
    if (m_bVisualizeTwistLimits && thisJoint.GetJointType() == WSkeletonJointType::SwingTwist)
    {
      auto& shape = m_AngleShapes.ExpandAndGetRef();
      shape.m_StartAngle = thisJoint.GetTwistLimitLow();
      shape.m_EndAngle = thisJoint.GetTwistLimitHigh();
      shape.m_Color = WMath::Lerp(WColor::DimGrey, WColor::LightPink, bHighlight ? 0.8f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.04f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot;

      const WColor hlT = WMath::Lerp(WColor::DimGrey, WColor::LightPink, bHighlight ? 1.0f : 0.4f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT2;
        cyl.m_Transform.m_vScale.Set(1);

        WVec3 vDir = cyl.m_Transform.m_qRotation * WVec3(1, 0, 0);
        vDir.Normalize();

        WVec3 vDirRef = shape.m_Transform.m_qRotation * qBoneDir * WVec3(0, 1, 0);
        vDirRef.Normalize();

        const WVec3 vRotDir = shape.m_Transform.m_qRotation * qBoneDir * WVec3(1, 0, 0);
        WQuat qRotRef = WQuat::MakeFromAxisAndAngle(vRotDir, thisJoint.GetTwistLimitCenterAngle());
        vDirRef = qRotRef * vDirRef;

        // if the current twist is outside the twist limit range, highlight the bone
        if (vDir.GetAngleBetween(vDirRef) > thisJoint.GetTwistLimitHalfAngle())
        {
          cyl.m_Color = WColor::Orange;
        }
      }
    }
  }
}

void WSkeletonComponent::VisualizeSkeletonDefaultState()
{
  if (!IsActiveAndInitialized())
    return;

  m_uiSkeletonChangeCounter = 0;

  if (m_hSkeleton.IsValid())
  {
    WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      m_uiSkeletonChangeCounter = pSkeleton->GetCurrentResourceChangeCounter();

      if (pSkeleton->GetDescriptor().m_Skeleton.GetJointCount() > 0)
      {
        ozz::vector<ozz::math::Float4x4> modelTransforms;
        modelTransforms.resize(pSkeleton->GetDescriptor().m_Skeleton.GetJointCount());

        {
          ozz::animation::LocalToModelJob job;
          job.input = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
          job.output = make_span(modelTransforms);
          job.skeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
          job.Run();
        }

        WMsgAnimationPoseUpdated msg;
        msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
        msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
        msg.m_ModelTransforms = WArrayPtr<const WMat4>(reinterpret_cast<const WMat4*>(&modelTransforms[0]), (WUInt32)modelTransforms.size());

        OnAnimationPoseUpdated(msg);
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

WDebugRendererLine& WSkeletonComponent::AddLine(const WVec3& vStart, const WVec3& vEnd, const WColor& color)
{
  auto& line = m_LinesSkeleton.ExpandAndGetRef();
  line.m_start = vStart;
  line.m_end = vEnd;
  line.m_startColor = color;
  line.m_endColor = color;
  return line;
}

void WSkeletonComponent::OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonComponent);

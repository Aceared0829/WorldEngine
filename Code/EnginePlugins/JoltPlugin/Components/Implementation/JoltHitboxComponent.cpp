#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Components/JoltHitboxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltHitboxComponent, 2, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("QueryShapeOnly", m_bQueryShapeOnly)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("UpdateThreshold", m_UpdateThreshold),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    W_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltHitboxComponent::WJoltHitboxComponent() = default;
WJoltHitboxComponent::~WJoltHitboxComponent() = default;

void WJoltHitboxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void WJoltHitboxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void WJoltHitboxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void WJoltHitboxComponent::OnDeactivated()
{
  if (m_uiObjectFilterID != WInvalidIndex)
  {
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void WJoltHitboxComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& ref_msg)
{
  if (m_UpdateThreshold.IsPositive())
  {
    const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (tNow - m_LastUpdate < m_UpdateThreshold)
      return;

    m_LastUpdate = tNow;
  }

  for (const auto& shape : m_Shapes)
  {
    WMat4 boneTrans;
    WQuat boneRot;
    ref_msg.ComputeFullBoneTransform(shape.m_uiAttachedToBone, boneTrans, boneRot);

    WTransform pose;
    pose.SetIdentity();
    pose.m_vPosition = boneTrans.GetTranslationVector() + boneRot * shape.m_vOffsetPos;
    pose.m_qRotation = boneRot * shape.m_qOffsetRot;

    WGameObject* pGO = nullptr;
    if (GetWorld()->TryGetObject(shape.m_hActorObject, pGO))
    {
      pGO->SetLocalPosition(pose.m_vPosition);
      pGO->SetLocalRotation(pose.m_qRotation);
    }
  }
}

void WJoltHitboxComponent::RecreatePhysicsShapes()
{
  WMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate = WTime::MakeZero();
}

void WJoltHitboxComponent::CreatePhysicsShapes(const WSkeletonResourceHandle& hSkeleton)
{
  WResourceLock<WSkeletonResource> pSkeleton(hSkeleton, WResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pSkeleton->GetDescriptor();

  W_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  m_Shapes.Reserve(desc.m_Geometry.GetCount());

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const WQuat qBoneDirAdjustment = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveX, srcBoneDir);

  const WQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  // the capsule should extend along X, but the capsule shape goes along Z
  const WQuat qRotZtoX = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(-90));
  const WQuat qRotYtoZ = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90));

  for (WUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  {
    const auto& geo = desc.m_Geometry[idx];

    if (geo.m_Type == WSkeletonJointGeometryType::None)
      continue;

    const WSkeletonJoint& joint = desc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    auto& shape = m_Shapes.ExpandAndGetRef();

    WGameObject* pGO = nullptr;

    {
      WGameObjectDesc god;
      god.m_bDynamic = true;
      god.m_hParent = GetOwner()->GetHandle();
      god.m_sName = joint.GetName();
      god.m_uiTeamID = GetOwner()->GetTeamID();

      shape.m_hActorObject = GetWorld()->CreateObject(god, pGO);

      if (m_bQueryShapeOnly)
      {
        WJoltQueryShapeActorComponent* pDynAct = nullptr;
        WJoltQueryShapeActorComponent::CreateComponent(pGO, pDynAct);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
      else
      {
        WJoltDynamicActorComponent* pDynAct = nullptr;
        WJoltDynamicActorComponent::CreateComponent(pGO, pDynAct);
        pDynAct->SetKinematic(true);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
    }

    shape.m_uiAttachedToBone = geo.m_uiAttachedToJoint;
    shape.m_vOffsetPos = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
    shape.m_qOffsetRot = qFinalBoneRot * geo.m_Transform.m_qRotation;


    if (geo.m_Type == WSkeletonJointGeometryType::Sphere)
    {
      WJoltShapeSphereComponent* pShapeComp = nullptr;
      WJoltShapeSphereComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
    }
    else if (geo.m_Type == WSkeletonJointGeometryType::Box)
    {
      WVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x;
      ext.y = geo.m_Transform.m_vScale.y;
      ext.z = geo.m_Transform.m_vScale.z;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * WVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      WJoltShapeBoxComponent* pShapeComp = nullptr;
      WJoltShapeBoxComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetHalfExtents(ext * 0.5f);
    }
    else if (geo.m_Type == WSkeletonJointGeometryType::Capsule)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotZtoX;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * WVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      WJoltShapeCapsuleComponent* pShapeComp = nullptr;
      WJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
    }
    else if (geo.m_Type == WSkeletonJointGeometryType::CapsuleSideways)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotYtoZ;

      WJoltShapeCapsuleComponent* pShapeComp = nullptr;
      WJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void WJoltHitboxComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltHitboxComponent);

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Components/JoltRagdollComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Body/BodyLockMulti.h>
#include <Physics/Collision/Shape/CompoundShape.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO
 * prevent crashes with zero bodies
 * external constraints
 * max force clamping / point vs area impulse ?
 * integrate root motion into controlled playback (?)
 */

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WJoltRagdollStartMode, 1)
  W_ENUM_CONSTANTS(WJoltRagdollStartMode::WithBindPose, WJoltRagdollStartMode::WithNextAnimPose, WJoltRagdollStartMode::WithCurrentMeshPose)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltRagdollAnimMode, 1)
  W_ENUM_CONSTANTS(WJoltRagdollAnimMode::Off, WJoltRagdollAnimMode::Limp, WJoltRagdollAnimMode::Powered, WJoltRagdollAnimMode::Controlled)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltRagdollComponent, 5, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    W_ENUM_ACCESSOR_PROPERTY("StartMode", WJoltRagdollStartMode, GetStartMode, SetStartMode),
    W_ENUM_ACCESSOR_PROPERTY("AnimMode", WJoltRagdollAnimMode, GetAnimMode, SetAnimMode),
    W_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new WDynamicEnumAttribute("PhysicsWeightCategory")),
    W_ACCESSOR_PROPERTY("WeightScale", GetWeight_Scale, SetWeight_Scale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("Mass", GetWeight_Mass, SetWeight_Mass)->AddAttributes(new WSuffixAttribute(" kg"), new WDefaultValueAttribute(50.0f), new WClampValueAttribute(1.0f, 1000.0f)),
    W_MEMBER_PROPERTY("StiffnessFactor", m_fStiffnessFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("OwnerVelocityScale", m_fOwnerVelocityScale)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("CenterPosition", m_vCenterPosition),
    W_MEMBER_PROPERTY("CenterVelocity", m_fCenterVelocity)->AddAttributes(new WDefaultValueAttribute(0.0f)),
    W_MEMBER_PROPERTY("CenterAngularVelocity", m_fCenterAngularVelocity)->AddAttributes(new WDefaultValueAttribute(0.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    W_MESSAGE_HANDLER(WMsgRetrieveBoneState, OnRetrieveBoneState),
    W_MESSAGE_HANDLER(WMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    W_MESSAGE_HANDLER(WMsgInjectPoseCommands, OnInjectPoseCommands),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Animation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    W_SCRIPT_FUNCTION_PROPERTY(SetInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    W_SCRIPT_FUNCTION_PROPERTY(AddInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    W_SCRIPT_FUNCTION_PROPERTY(SetJointTypeOverride, In, "sJointName", In, "overrideType"),
    W_SCRIPT_FUNCTION_PROPERTY(SetJointMotorStrength, In, "fStrength"),
    W_SCRIPT_FUNCTION_PROPERTY(GetJointMotorStrength),
    W_SCRIPT_FUNCTION_PROPERTY(FadeJointMotorStrength, In, "fTargetStrength", In, "tDuration"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WJoltRagdollComponentManager::WJoltRagdollComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltRagdollComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltRagdollComponentManager::~WJoltRagdollComponentManager() = default;

void WJoltRagdollComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltRagdollComponentManager::Update, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WJoltRagdollComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  W_PROFILE_SCOPE("UpdateRagdolls");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  for (auto it : pModule->GetActiveRagdolls())
  {
    WJoltRagdollComponent* pComponent = it.Key();

    pComponent->Update(false);
  }

  for (WJoltRagdollComponent* pComponent : pModule->GetRagdollsPutToSleep())
  {
    pComponent->Update(true);
  }
}

void WJoltRagdollComponentManager::DriveAnimatedRagdolls(WTime deltaTime)
{
  W_PROFILE_SCOPE("DriveAnimatedRagdolls");

  for (auto it = GetComponents(0); it.IsValid(); it.Next())
  {
    it->DriveAnimated(deltaTime);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WJoltRagdollComponent::WJoltRagdollComponent() = default;
WJoltRagdollComponent::~WJoltRagdollComponent() = default;

void WJoltRagdollComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();


  s << m_StartMode;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_fOwnerVelocityScale;
  s << m_fCenterVelocity;
  s << m_fCenterAngularVelocity;
  s << m_vCenterPosition;
  s << m_uiWeightCategory;
  s << (float)m_fWeightScale;
  s << (float)m_fWeightMass;
  s << m_fStiffnessFactor;
  s << m_AnimMode;
}

void WJoltRagdollComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  W_ASSERT_DEBUG(uiVersion >= 4, "Outdated version, please re-transform asset.");
  if (uiVersion < 4)
    return;

  s >> m_StartMode;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_fOwnerVelocityScale;
  s >> m_fCenterVelocity;
  s >> m_fCenterAngularVelocity;
  s >> m_vCenterPosition;
  s >> m_uiWeightCategory;

  {
    float f;

    s >> f;
    m_fWeightScale = f;

    s >> f;
    m_fWeightMass = f;
  }

  s >> m_fStiffnessFactor;

  if (uiVersion >= 5)
  {
    s >> m_AnimMode;
  }
}

void WJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_pJoltWorldModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  if (m_StartMode == WJoltRagdollStartMode::WithBindPose)
  {
    CreateLimbsFromBindPose();
  }
  if (m_StartMode == WJoltRagdollStartMode::WithCurrentMeshPose)
  {
    CreateLimbsFromCurrentMeshPose();
  }
}

void WJoltRagdollComponent::OnDeactivated()
{
  DestroyAllLimbs();

  if (m_pSkeletonPose)
  {
    auto pMan = static_cast<WJoltRagdollComponentManager*>(GetOwningManager());

    W_LOCK(pMan->m_SkeletonsMutex);
    pMan->m_FreeSkeletonPoses.PushBack(std::move(m_pSkeletonPose));
    m_pSkeletonPose.Clear();
  }

  SUPER::OnDeactivated();
}

void WJoltRagdollComponent::Update(bool bForce)
{
  if (!HasCreatedLimbs())
    return;

  if (m_AnimMode != WJoltRagdollAnimMode::Powered && m_bIsPowered)
  {
    ResetJointMotors();
  }

  if (m_MotorLerpDuration.IsPositive())
  {
    const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
    const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();
    const WTime tStart = tNow - tDiff;
    const WTime tFinish = tStart + m_MotorLerpDuration;

    const float fLerpFactor = WMath::Saturate(WMath::Unlerp(tStart.GetSeconds(), tFinish.GetSeconds(), tNow.GetSeconds()));
    const float fNewStrength = WMath::Lerp(m_fMotorStrength, m_fMotorTargetStrength, fLerpFactor);

    if (m_fMotorStrength != fNewStrength)
    {
      m_fMotorStrength = fNewStrength;
      m_MotorLerpDuration -= tDiff;

      if (m_MotorLerpDuration.IsNegative())
        m_MotorLerpDuration = WTime::MakeZero();

      ApplyJointMotorStrength(fNewStrength);
    }
  }

  if (m_pSkeletonPose && (m_AnimMode != WJoltRagdollAnimMode::Controlled) && (m_AnimMode != WJoltRagdollAnimMode::Powered || m_fMotorStrength <= 0.0f))
  {
    auto pMan = static_cast<WJoltRagdollComponentManager*>(GetOwningManager());

    W_LOCK(pMan->m_SkeletonsMutex);
    pMan->m_FreeSkeletonPoses.PushBack(std::move(m_pSkeletonPose));
    m_pSkeletonPose.Clear();
  }

  const WVisibilityState::Enum visState = GetOwner()->GetVisibilityState();
  if (!bForce && visState != WVisibilityState::Direct)
  {
    m_ElapsedTimeSinceUpdate += WClock::GetGlobalClock()->GetTimeDiff();

    if (visState == WVisibilityState::Indirect && m_ElapsedTimeSinceUpdate < WTime::MakeFromMilliseconds(200))
    {
      // when the ragdoll is only visible by shadows or reflections, update it infrequently
      return;
    }

    if (visState == WVisibilityState::Invisible && m_ElapsedTimeSinceUpdate < WTime::MakeFromMilliseconds(500))
    {
      // when the ragdoll is entirely invisible, update it very rarely
      return;
    }
  }

  const WVec3 vRootPos = RetrieveRagdollPose();
  GetOwner()->SetGlobalPosition(vRootPos);

  SendAnimationPoseMsg();

  m_ElapsedTimeSinceUpdate = WTime::MakeZero();
}

void WJoltRagdollComponent::DriveAnimated(WTime deltaTime)
{
  if (m_pSkeletonPose)
  {
    m_pSkeletonPose->CalculateJointStates();

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    for (auto& joint : m_pSkeletonPose->GetJoints())
    {
      joint.mRotation = joint.mRotation.Normalized();
    }
#endif

    if (m_AnimMode == WJoltRagdollAnimMode::Controlled)
    {
      m_pRagdoll->DriveToPoseUsingKinematics(*m_pSkeletonPose, deltaTime.AsFloatInSeconds());
    }
    else if (m_AnimMode == WJoltRagdollAnimMode::Powered && m_fMotorStrength > 0.0f)
    {
      m_bIsPowered = true;
      m_pRagdoll->DriveToPoseUsingMotors(*m_pSkeletonPose);
    }
    else
    {
      m_pSkeletonPose.Clear();
    }
  }
}

WResult WJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    WMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  if (!m_hSkeleton.IsValid())
  {
    WLog::Error("No skeleton available for ragdoll on object '{}'.", GetOwner()->GetName());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

bool WJoltRagdollComponent::HasCreatedLimbs() const
{
  return m_pRagdoll != nullptr;
}

void WJoltRagdollComponent::CreateLimbsFromBindPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  m_CurrentLimbTransforms.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto ComputeFullJointTransform = [&](WUInt32 uiJointIdx, auto self) -> WMat4
  {
    const auto& joint = desc.m_Skeleton.GetJointByIndex(uiJointIdx);
    const WMat4 jointTransform = joint.GetRestPoseLocalTransform().GetAsMat4();

    if (joint.GetParentIndex() != WInvalidJointIndex)
    {
      const WMat4 parentTransform = self(joint.GetParentIndex(), self);

      return parentTransform * jointTransform;
    }

    return jointTransform;
  };

  for (WUInt32 i = 0; i < m_CurrentLimbTransforms.GetCount(); ++i)
  {
    m_CurrentLimbTransforms[i] = ComputeFullJointTransform(i, ComputeFullJointTransform);
  }

  WMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void WJoltRagdollComponent::CreateLimbsFromCurrentMeshPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  WAnimatedMeshComponent* pMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType<WAnimatedMeshComponent>(pMesh))
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  WTransform tRoot;
  pMesh->RetrievePose(m_CurrentLimbTransforms, tRoot, pSkeleton->GetDescriptor().m_Skeleton);

  WMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &tRoot;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void WJoltRagdollComponent::DestroyAllLimbs()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiJoltUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_pJoltUserData = nullptr;
  }

  m_CurrentLimbTransforms.Clear();
  m_Limbs.Clear();
}

void WJoltRagdollComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  if (!m_pRagdoll)
    return;

  for (WUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    m_pJoltWorldModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void WJoltRagdollComponent::SetStartMode(WEnum<WJoltRagdollStartMode> mode)
{
  if (m_StartMode == mode)
    return;

  m_StartMode = mode;
}

void WJoltRagdollComponent::SetAnimMode(WEnum<WJoltRagdollAnimMode> mode)
{
  if (m_AnimMode == mode)
    return;

  m_AnimMode = mode;

  if (m_pRagdoll)
  {
    if (m_AnimMode == WJoltRagdollAnimMode::Controlled)
    {
      for (WUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
      {
        // in the 'Controlled' mode, disable gravity, so that it doesn't affect the pose
        m_pJoltWorldModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), 0.0f);
      }
    }
    else
    {
      for (WUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
      {
        m_pJoltWorldModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
      }
    }
  }
}

void WJoltRagdollComponent::OnMsgPhysicsAddImpulse(WMsgPhysicsAddImpulse& ref_msg)
{
  const float fImpulse = WJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(ref_msg.m_uiImpulseType, m_uiWeightCategory);

  if (!HasCreatedLimbs())
  {
    m_vInitialImpulsePosition += ref_msg.m_vGlobalPosition;
    m_vInitialImpulseDirection += ref_msg.m_vImpulse * fImpulse;
    m_uiNumInitialImpulses++;
    return;
  }

  // TODO: normalize by number of limbs
  const WUInt32 uiBodyId = reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF;
  GetWorld()->GetModule<WJoltWorldModule>()->AddImpulse(uiBodyId, ref_msg.m_vImpulse * fImpulse, ref_msg.m_vGlobalPosition);
}

void WJoltRagdollComponent::SetInitialImpulse(const WVec3& vPosition, const WVec3& vDirectionAndStrength)
{
  if (vDirectionAndStrength.IsZero())
  {
    m_vInitialImpulsePosition.SetZero();
    m_vInitialImpulseDirection.SetZero();
    m_uiNumInitialImpulses = 0;
  }
  else
  {
    m_vInitialImpulsePosition = vPosition;
    m_vInitialImpulseDirection = vDirectionAndStrength;
    m_uiNumInitialImpulses = 1;
  }
}

void WJoltRagdollComponent::AddInitialImpulse(const WVec3& vPosition, const WVec3& vDirectionAndStrength)
{
  m_vInitialImpulsePosition += vPosition;
  m_vInitialImpulseDirection += vDirectionAndStrength;
  m_uiNumInitialImpulses++;
}

void WJoltRagdollComponent::SetJointTypeOverride(WStringView sJointName, WEnum<WSkeletonJointType> type)
{
  const WTempHashedString sJointNameHashed(sJointName);

  for (WUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == sJointNameHashed)
    {
      m_JointOverrides[i].m_JointType = type;
      return;
    }
  }

  auto& jo = m_JointOverrides.ExpandAndGetRef();
  jo.m_sJointName = sJointNameHashed;
  jo.m_JointType = type;
}

void WJoltRagdollComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (HasCreatedLimbs())
  {
    // Note: if this code is reached, although the ragdoll is supposed to use the animation (controlled or powered anim mode)
    // then the component that generates the animation doesn't have "Apply IK" enabled, ie it doesn't send WMsgInjectPoseCommands
    ref_poseMsg.m_bContinueAnimating = false;

    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  if (m_StartMode != WJoltRagdollStartMode::WithNextAnimPose)
    return;

  m_CurrentLimbTransforms = ref_poseMsg.m_ModelTransforms;

  CreateLimbsFromPose(ref_poseMsg);
}

void WJoltRagdollComponent::OnRetrieveBoneState(WMsgRetrieveBoneState& ref_msg) const
{
  if (!HasCreatedLimbs())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (WUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    WMat4 mJoint = m_CurrentLimbTransforms[uiJointIdx];

    const auto& joint = skeleton.GetJointByIndex(uiJointIdx);
    const WUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != WInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const WMat4 mParent = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t = ref_msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

static void ComputeFullBoneTransform(const WMat4& mRootTransform, const WMat4& mModelTransform, WTransform& out_transform)
{
  WMat4 mFullTransform = mRootTransform * mModelTransform;

  out_transform.m_qRotation.ReconstructFromMat4(mFullTransform);
  out_transform.m_vScale.Set(1);
  out_transform.m_vPosition = mFullTransform.GetTranslationVector();
}

void WJoltRagdollComponent::OnInjectPoseCommands(WMsgInjectPoseCommands& ref_msg)
{
  if (m_AnimMode == WJoltRagdollAnimMode::Limp)
    return;

  if (!HasCreatedLimbs())
    return;

  // if we are already past this, just return
  if (ref_msg.m_uiOrderNow > 0xFF00)
    return;

  // if we haven't reached this yet, put it in the queue
  if (ref_msg.m_uiOrderNow < 0xFF00)
  {
    ref_msg.m_uiOrderNext = WMath::Min<WUInt16>(ref_msg.m_uiOrderNext, 0xFF00);
    return;
  }

  if (m_AnimMode == WJoltRagdollAnimMode::Powered && m_fMotorStrength == 0.0f)
  {
    // basically the same as Limp mode, but we don't want to deactivate animations, because motor strength can still be changed
    return;
  }

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const WMat4 mRootTransform = pSkeleton->GetDescriptor().m_RootTransform.GetAsMat4();

  const WTransform tGlobal = GetOwner()->GetGlobalTransform();
  const auto& curPose = ref_msg.m_pGenerator->GetCurrentPose();

  if (m_pSkeletonPose == nullptr)
  {
    auto pMan = static_cast<WJoltRagdollComponentManager*>(GetOwningManager());
    W_LOCK(pMan->m_SkeletonsMutex);
    if (!pMan->m_FreeSkeletonPoses.IsEmpty())
    {
      m_pSkeletonPose = std::move(pMan->m_FreeSkeletonPoses.PeekBack());
      pMan->m_FreeSkeletonPoses.PopBack();
    }
    else
    {
      m_pSkeletonPose = W_NEW(WFoundation::GetAlignedAllocator(), JPH::SkeletonPose);
    }
  }

  m_pSkeletonPose->SetSkeleton(m_pRagdoll->GetRagdollSettings()->GetSkeleton());

  WVec3 vRootOffset(0);

  for (WUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex != WInvalidJointIndex)
    {
      JPH::Mat44& jointMat = m_pSkeletonPose->GetJointMatrix(m_Limbs[uiLimbIdx].m_uiPartIndex);

      WTransform trans;
      ComputeFullBoneTransform(mRootTransform, curPose[uiLimbIdx], trans);

      trans = WTransform::MakeGlobalTransform(tGlobal, trans);

      WMat4 mGlobal = trans.GetAsMat4();
      jointMat = JPH::Mat44::sLoadFloat4x4((const JPH::Float4*)mGlobal.m_fElementsCM);
    }
  }

  m_pSkeletonPose->SetRootOffset(WJoltConversionUtils::ToVec3(vRootOffset));
}

void WJoltRagdollComponent::SetJointMotorStrength(float fStrength)
{
  if (m_fMotorStrength != fStrength)
  {
    ApplyJointMotorStrength(fStrength);
  }

  m_fMotorStrength = fStrength;
  m_fMotorTargetStrength = fStrength;
  m_MotorLerpDuration = WTime::MakeZero();
}

float WJoltRagdollComponent::GetJointMotorStrength() const
{
  return m_fMotorStrength;
}

void WJoltRagdollComponent::FadeJointMotorStrength(float fTargetStrength, WTime duration)
{
  m_fMotorTargetStrength = fTargetStrength;
  m_MotorLerpDuration = duration;
}

void WJoltRagdollComponent::SendAnimationPoseMsg()
{
  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const WTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;

  WMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_CurrentLimbTransforms;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

  GetOwner()->SendMessageRecursive(poseMsg);
}

WVec3 WJoltRagdollComponent::RetrieveRagdollPose()
{
  const float fLerpToPos = (m_AnimMode != WJoltRagdollAnimMode::Controlled) ? 0.1f : 0.0f;

  const JPH::RVec3 vCurPosition = WJoltConversionUtils::ToVec3(GetOwner()->GetGlobalPosition());

  const int body_count = (int)m_pRagdoll->GetBodyCount();
  JPH::BodyLockMultiRead lock(static_cast<const JPH::BodyLockInterface&>(m_pJoltWorldModule->GetJoltSystem()->GetBodyLockInterface()), m_pRagdoll->GetBodyIDs().data(), body_count);

  const JPH::Body* root = lock.GetBody(0);
  JPH::RMat44 root_transform = root->GetWorldTransform();
  JPH::RVec3 vRootOffset = root_transform.GetTranslation();
  vRootOffset = vCurPosition + (vRootOffset - vCurPosition) * fLerpToPos; // interpolate the object position towards the root bone position


  const WVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = WMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const WSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const WTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const WQuat qGlobalRot = GetOwner()->GetGlobalRotation();
  const WMat4 mInvScale = WMat4::MakeScaling(WVec3(1.0f).CompDiv(WVec3(fObjectScale)));

  const WMat4 mInv = m_mInvSkeletonRootTransform * qGlobalRot.GetInverse().GetAsMat4() * mInvScale;
  const WMat4 mScale = WMat4::MakeScaling(rootTransform.m_vScale * fObjectScale);

  WTempHybridArray<WMat4, 128> relativeTransforms;

  {
    // m_CurrentLimbTransforms is stored in model space
    // for bones that don't have their own shape in the ragdoll,
    // we don't get a new transform from the ragdoll, but we still must update them,
    // if there is a parent bone in the ragdoll, otherwise they don't move along as expected
    // therefore we compute their relative transform here
    // and then later we take their new parent transform (which may come from the ragdoll)
    // to set their final new transform

    for (WUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
    {
      if (m_Limbs[uiLimbIdx].m_uiPartIndex != WInvalidJointIndex)
        continue;

      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const WUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx == WInvalidJointIndex)
        continue;

      const WMat4 mJoint = m_CurrentLimbTransforms[uiLimbIdx];

      // remove the parent transform to get the pure local transform
      const WMat4 mParentInv = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      relativeTransforms.PushBack(mParentInv * mJoint);
    }
  }

  WUInt32 uiNextRelativeIdx = 0;
  for (WUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex == WInvalidJointIndex)
    {
      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const WUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx != WInvalidJointIndex)
      {
        m_CurrentLimbTransforms[uiLimbIdx] = m_CurrentLimbTransforms[uiParentIdx] * relativeTransforms[uiNextRelativeIdx];
        ++uiNextRelativeIdx;
      }
    }
    else
    {
      const JPH::Body* pBody = lock.GetBody(m_Limbs[uiLimbIdx].m_uiPartIndex);
      const JPH::RMat44 transform = pBody->GetWorldTransform();
      const JPH::Mat44 jointMatrix = JPH::Mat44(transform.GetColumn4(0), transform.GetColumn4(1), transform.GetColumn4(2), JPH::Vec4(JPH::Vec3(transform.GetTranslation() - vRootOffset), 1));

      const WMat4& mPose = (const WMat4&)jointMatrix;

      m_CurrentLimbTransforms[uiLimbIdx] = (mInv * mPose) * mScale;
    }
  }

  return WJoltConversionUtils::ToVec3(vRootOffset);
}

void WJoltRagdollComponent::CreateLimbsFromPose(const WMsgAnimationPoseUpdated& pose)
{
  W_ASSERT_DEBUG(!HasCreatedLimbs(), "Limbs are already created.");

  if (EnsureSkeletonIsKnown().Failed())
    return;

  const WVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = WMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  m_uiObjectFilterID = m_pJoltWorldModule->CreateObjectFilterID();
  m_uiJoltUserDataIndex = m_pJoltWorldModule->AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);

  WResourceLock<WSkeletonResource> pSkeletonResource(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);

  m_mInvSkeletonRootTransform = pSkeletonResource->GetDescriptor().m_RootTransform.GetAsMat4().GetInverse();

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  JPH::Ref<JPH::RagdollSettings> ragdollSettings = new JPH::RagdollSettings();

  ragdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  ragdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  ragdollSettings->mSkeleton->GetJoints().reserve(ragdollSettings->mParts.size());

  CreateAllLimbs(*pSkeletonResource.GetPointer(), pose, *m_pJoltWorldModule, fObjectScale, ragdollSettings);

  {
    const float fInitialMass = WJoltCore::GetWeightCategoryConfig().GetMassForWeightCategory(m_uiWeightCategory, 50.0f, m_fWeightMass, m_fWeightScale);

    ApplyBodyMass(ragdollSettings, fInitialMass);
  }

  SetupLimbJoints(pSkeletonResource.GetPointer(), ragdollSettings);
  ApplyPartInitialVelocity(ragdollSettings);

  if (m_bSelfCollision)
  {
    // enables collisions between all bodies except the ones that are directly connected to each other
    ragdollSettings->DisableParentChildCollisions();
  }

  ragdollSettings->Stabilize();
  ragdollSettings->CalculateBodyIndexToConstraintIndex();
  ragdollSettings->CalculateConstraintIndexToBodyIdxPair();

  m_pRagdoll = ragdollSettings->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<WUInt64>(m_pJoltUserData), m_pJoltWorldModule->GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  ApplyInitialImpulse(*m_pJoltWorldModule, pSkeletonResource->GetDescriptor().m_fMaxImpulse);
}

void WJoltRagdollComponent::ConfigureRagdollPart(void* pRagdollSettingsPart, const WTransform& globalTransform, WUInt8 uiCollisionLayer, WJoltWorldModule& worldModule)
{
  JPH::RagdollSettings::Part* pPart = reinterpret_cast<JPH::RagdollSettings::Part*>(pRagdollSettingsPart);

  pPart->mPosition = WJoltConversionUtils::ToVec3(globalTransform.m_vPosition);
  pPart->mRotation = WJoltConversionUtils::ToQuat(globalTransform.m_qRotation).Normalized();
  pPart->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pPart->mGravityFactor = m_AnimMode == WJoltRagdollAnimMode::Controlled ? 0.0f : m_fGravityFactor;
  pPart->mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
  pPart->mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(uiCollisionLayer, WJoltBroadphaseLayer::Ragdoll);
  pPart->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  pPart->mCollisionGroup.SetGroupFilter(worldModule.GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below
}

void WJoltRagdollComponent::ApplyPartInitialVelocity(JPH::RagdollSettings* pRagdollSettings)
{
  JPH::Vec3 vCommonVelocity = WJoltConversionUtils::ToVec3(GetOwner()->GetLinearVelocity() * m_fOwnerVelocityScale);
  const JPH::Vec3 vCenterPos = WJoltConversionUtils::ToVec3(GetOwner()->GetGlobalTransform() * m_vCenterPosition);

  WCoordinateSystem coord;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coord);
  WRandom& rng = GetOwner()->GetWorld()->GetRandomNumberGenerator();

  for (JPH::RagdollSettings::Part& part : pRagdollSettings->mParts)
  {
    part.mLinearVelocity = vCommonVelocity;

    if (m_fCenterVelocity != 0.0f)
    {
      const JPH::Vec3 vVelocityDir = (part.mPosition - vCenterPos).NormalizedOr(JPH::Vec3::sZero());
      part.mLinearVelocity += vVelocityDir * WMath::Min(part.mMaxLinearVelocity, m_fCenterVelocity);
    }

    if (m_fCenterAngularVelocity != 0.0f)
    {
      const WVec3 vVelocityDir = WJoltConversionUtils::ToVec3(part.mPosition - vCenterPos);
      WVec3 vRotationDir = vVelocityDir.CrossRH(coord.m_vUpDir);
      vRotationDir.NormalizeIfNotZero(coord.m_vUpDir).IgnoreResult();

      WVec3 vRotationAxis = WVec3::MakeRandomDeviation(rng, WAngle::MakeFromDegree(30.0f), vRotationDir);
      vRotationAxis *= rng.Bool() ? 1.0f : -1.0f;

      float fSpeed = rng.FloatVariance(m_fCenterAngularVelocity, 0.5f);
      fSpeed = WMath::Min(fSpeed, part.mMaxAngularVelocity * 0.95f);

      part.mAngularVelocity = WJoltConversionUtils::ToVec3(vRotationAxis) * fSpeed;
    }
  }
}

void WJoltRagdollComponent::ApplyInitialImpulse(WJoltWorldModule& worldModule, float fMaxImpulse)
{
  if (m_uiNumInitialImpulses == 0)
    return;

  if (m_uiNumInitialImpulses > 1)
  {
    WLog::Info("Impulses: {} - {}", m_uiNumInitialImpulses, m_vInitialImpulseDirection.GetLength());
  }

  auto pJoltSystem = worldModule.GetJoltSystem();

  m_vInitialImpulsePosition /= m_uiNumInitialImpulses;

  float fImpulse = m_vInitialImpulseDirection.GetLength();

  if (fImpulse > fMaxImpulse)
  {
    fImpulse = fMaxImpulse;
    m_vInitialImpulseDirection.SetLength(fImpulse).AssertSuccess();
  }

  const JPH::Vec3 vImpulsePosition = WJoltConversionUtils::ToVec3(m_vInitialImpulsePosition);
  float fLowestDistanceSqr = 100000;

  JPH::BodyID closestBody;

  for (WUInt32 uiBodyIdx = 0; uiBodyIdx < m_pRagdoll->GetBodyCount(); ++uiBodyIdx)
  {
    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(uiBodyIdx);
    JPH::BodyLockRead bodyRead(pJoltSystem->GetBodyLockInterface(), bodyId);

    const float fDistanceToImpulseSqr = (bodyRead.GetBody().GetPosition() - vImpulsePosition).LengthSq();

    if (fDistanceToImpulseSqr < fLowestDistanceSqr)
    {
      fLowestDistanceSqr = fDistanceToImpulseSqr;
      closestBody = bodyId;
    }
  }

  if (pJoltSystem->GetBodyInterface().IsAdded(closestBody))
  {
    pJoltSystem->GetBodyInterface().AddImpulse(closestBody, WJoltConversionUtils::ToVec3(m_vInitialImpulseDirection), vImpulsePosition);
  }
}

void WJoltRagdollComponent::ResetJointMotors()
{
  if (!m_bIsPowered)
    return;

  m_bIsPowered = false;

  if (!m_pRagdoll)
    return;

  for (int i = 0; i < (int)m_pRagdoll->GetConstraintCount(); ++i)
  {
    JPH::TwoBodyConstraint* pConstraint = m_pRagdoll->GetConstraint(i);

    JPH::EConstraintSubType subType = pConstraint->GetSubType();
    if (subType == JPH::EConstraintSubType::SwingTwist)
    {
      JPH::SwingTwistConstraint* pStConstraint = static_cast<JPH::SwingTwistConstraint*>(pConstraint);
      pStConstraint->SetSwingMotorState(JPH::EMotorState::Off);
      pStConstraint->SetTwistMotorState(JPH::EMotorState::Off);
    }
  }
}

void WJoltRagdollComponent::ApplyJointMotorStrength(float fStrength)
{
  if (!m_pRagdoll)
    return;

  for (int i = 0; i < (int)m_pRagdoll->GetConstraintCount(); ++i)
  {
    JPH::TwoBodyConstraint* pConstraint = m_pRagdoll->GetConstraint(i);

    JPH::EConstraintSubType subType = pConstraint->GetSubType();
    if (subType == JPH::EConstraintSubType::SwingTwist)
    {
      JPH::SwingTwistConstraint* pStConstraint = static_cast<JPH::SwingTwistConstraint*>(pConstraint);

      pStConstraint->GetSwingMotorSettings().SetForceLimit(m_fMotorStrength);
      pStConstraint->GetTwistMotorSettings().SetForceLimit(m_fMotorStrength);

      // torque is needed for the 'powered' animation mode
      pStConstraint->GetSwingMotorSettings().SetTorqueLimit(m_fMotorStrength);
      pStConstraint->GetTwistMotorSettings().SetTorqueLimit(m_fMotorStrength);
    }
  }
}

void WJoltRagdollComponent::ApplyBodyMass(JPH::RagdollSettings* pRagdollSettings, float fMass)
{
  if (fMass <= 0.0f)
    return;

  float fPartMass = fMass / pRagdollSettings->mParts.size();

  for (auto& part : pRagdollSettings->mParts)
  {
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mMassPropertiesOverride.mMass = fPartMass;
  }
}

void WJoltRagdollComponent::ComputeLimbModelSpaceTransform(WTransform& transform, const WMsgAnimationPoseUpdated& pose, WUInt32 uiPoseJointIndex)
{
  WMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiPoseJointIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void WJoltRagdollComponent::ComputeLimbGlobalTransform(WTransform& transform, const WMsgAnimationPoseUpdated& pose, WUInt32 uiPoseJointIndex)
{
  WTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiPoseJointIndex);
  transform = WTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void WJoltRagdollComponent::CreateAllLimbs(const WSkeletonResource& skeletonResource, const WMsgAnimationPoseUpdated& pose, WJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
{
  WMap<WUInt16, LimbConstructionInfo> limbConstructionInfos(WTempAllocator::Get());
  limbConstructionInfos.FindOrAdd(WInvalidJointIndex); // dummy root link

  WUInt16 uiLastLimbIdx = WInvalidJointIndex;
  WTempHybridArray<const WSkeletonResourceGeometry*, 8> geometries;

  for (const auto& geo : skeletonResource.GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == WSkeletonJointGeometryType::None)
      continue;

    if (geo.m_uiAttachedToJoint != uiLastLimbIdx)
    {
      CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale, pRagdollSettings);
      geometries.Clear();
      uiLastLimbIdx = geo.m_uiAttachedToJoint;
    }

    geometries.PushBack(&geo);
  }

  CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale, pRagdollSettings);
}

void WJoltRagdollComponent::CreateLimb(const WSkeletonResource& skeletonResource, WMap<WUInt16, LimbConstructionInfo>& limbConstructionInfos, WArrayPtr<const WSkeletonResourceGeometry*> geometries, const WMsgAnimationPoseUpdated& pose, WJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
{
  if (geometries.IsEmpty())
    return;

  const WSkeleton& skeleton = skeletonResource.GetDescriptor().m_Skeleton;

  const WUInt16 uiThisJointIdx = geometries[0]->m_uiAttachedToJoint;
  const WSkeletonJoint& thisLimbJoint = skeleton.GetJointByIndex(uiThisJointIdx);
  WUInt16 uiParentJointIdx = thisLimbJoint.GetParentIndex();

  // find the parent joint that is also part of the ragdoll
  while (!limbConstructionInfos.Contains(uiParentJointIdx))
  {
    uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
  }
  // now uiParentJointIdx is either the index of a limb that has been created before, or WInvalidJointIndex

  LimbConstructionInfo& thisLimbInfo = limbConstructionInfos[uiThisJointIdx];
  const LimbConstructionInfo& parentLimbInfo = limbConstructionInfos[uiParentJointIdx];

  thisLimbInfo.m_uiJoltPartIndex = (WUInt16)pRagdollSettings->mParts.size();
  pRagdollSettings->mParts.resize(pRagdollSettings->mParts.size() + 1);

  m_Limbs[uiThisJointIdx].m_uiPartIndex = thisLimbInfo.m_uiJoltPartIndex;

  pRagdollSettings->mSkeleton->AddJoint(thisLimbJoint.GetName().GetData(), parentLimbInfo.m_uiJoltPartIndex != WInvalidJointIndex ? parentLimbInfo.m_uiJoltPartIndex : -1);

  ComputeLimbGlobalTransform(thisLimbInfo.m_GlobalTransform, pose, uiThisJointIdx);
  ConfigureRagdollPart(&pRagdollSettings->mParts[thisLimbInfo.m_uiJoltPartIndex], thisLimbInfo.m_GlobalTransform, thisLimbJoint.GetCollisionLayer(), worldModule);
  CreateAllLimbGeoShapes(thisLimbInfo, geometries, thisLimbJoint, skeletonResource, fObjectScale, pRagdollSettings);
}

JPH::Shape* WJoltRagdollComponent::CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const WSkeletonResourceGeometry& geo, const WJoltMaterial* pJoltMaterial, const WQuat& qBoneDirAdjustment, const WTransform& skeletonRootTransform, WTransform& out_shapeTransform, float fObjectScale)
{
  out_shapeTransform.SetIdentity();
  out_shapeTransform.m_vPosition = qBoneDirAdjustment * geo.m_Transform.m_vPosition * fObjectScale;
  out_shapeTransform.m_qRotation = qBoneDirAdjustment * geo.m_Transform.m_qRotation;

  JPH::Ref<JPH::Shape> pShape;

  switch (geo.m_Type)
  {
    case WSkeletonJointGeometryType::Sphere:
    {
      JPH::SphereShapeSettings shape;
      shape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      pShape = shape.Create().Get();
    }
    break;

    case WSkeletonJointGeometryType::Box:
    {
      JPH::BoxShapeSettings shape;
      shape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      WVec3 vHalfSize = geo.m_Transform.m_vScale * 0.5f * fObjectScale;
      vHalfSize.x = WMath::Max(vHalfSize.x, JPH::cDefaultConvexRadius);
      vHalfSize.y = WMath::Max(vHalfSize.y, JPH::cDefaultConvexRadius);
      vHalfSize.z = WMath::Max(vHalfSize.z, JPH::cDefaultConvexRadius);
      shape.mHalfExtent = WJoltConversionUtils::ToVec3(vHalfSize);

      out_shapeTransform.m_vPosition += qBoneDirAdjustment * WVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case WSkeletonJointGeometryType::Capsule:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation * qRot;
      out_shapeTransform.m_vPosition += qBoneDirAdjustment * WVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case WSkeletonJointGeometryType::CapsuleSideways:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      // WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation; // *qRot;
      // out_shapeTransform.m_vPosition += qBoneDirAdjustment * WVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case WSkeletonJointGeometryType::ConvexMesh:
    {
      // convex mesh vertices are in "global space" of the mesh file format
      // so first move them into global space of the W convention (skeletonRootTransform)
      // then move them to the global position of the ragdoll object
      // then apply the inverse global transform of the limb, to move everything into local space of the limb

      out_shapeTransform = limbConstructionInfo.m_GlobalTransform.GetInverse() * GetOwner()->GetGlobalTransform() * skeletonRootTransform;
      out_shapeTransform.m_vPosition *= fObjectScale;

      WTempHybridArray<JPH::Vec3, 256> verts;
      verts.SetCountUninitialized(geo.m_VertexPositions.GetCount());

      for (WUInt32 i = 0; i < verts.GetCount(); ++i)
      {
        verts[i] = WJoltConversionUtils::ToVec3(geo.m_VertexPositions[i] * fObjectScale);
      }

      JPH::ConvexHullShapeSettings shape(verts.GetData(), (int)verts.GetCount());
      shape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;

      const auto shapeRes = shape.Create();

      if (shapeRes.HasError())
      {
        WLog::Error("Cooking convex ragdoll piece failed: {}", shapeRes.GetError().c_str());
        return nullptr;
      }

      pShape = shapeRes.Get();
    }
    break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  pShape->AddRef();
  return pShape;
}

void WJoltRagdollComponent::CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, WArrayPtr<const WSkeletonResourceGeometry*> geometries, const WSkeletonJoint& thisLimbJoint, const WSkeletonResource& skeletonResource, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
{
  const WJoltMaterial* pJoltMaterial = WJoltCore::GetDefaultMaterial();

  if (thisLimbJoint.GetSurface().IsValid())
  {
    WResourceLock<WSurfaceResource> pSurface(thisLimbJoint.GetSurface(), WResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      pJoltMaterial = static_cast<WJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  const WTransform& skeletonRootTransform = skeletonResource.GetDescriptor().m_RootTransform;

  const auto srcBoneDir = skeletonResource.GetDescriptor().m_Skeleton.m_BoneDirection;
  const WQuat qBoneDirAdjustment = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveX, srcBoneDir);

  JPH::RagdollSettings::Part* pBodyDesc = &pRagdollSettings->mParts[limbConstructionInfo.m_uiJoltPartIndex];

  if (geometries.GetCount() > 1)
  {
    JPH::StaticCompoundShapeSettings compound;

    for (const WSkeletonResourceGeometry* pGeo : geometries)
    {
      WTransform shapeTransform;
      if (JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *pGeo, pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale))
      {
        compound.AddShape(WJoltConversionUtils::ToVec3(shapeTransform.m_vPosition), WJoltConversionUtils::ToQuat(shapeTransform.m_qRotation), pSubShape);
        pSubShape->Release(); // had to manual AddRef once
      }
    }

    const auto compoundRes = compound.Create();
    if (!compoundRes.IsValid())
    {
      WLog::Error("Creating a compound shape for a ragdoll failed: {}", compoundRes.GetError().c_str());
      return;
    }

    pBodyDesc->SetShape(compoundRes.Get());
  }
  else
  {
    WTransform shapeTransform;
    JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *geometries[0], pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale);

    if (!shapeTransform.IsEqual(WTransform::MakeIdentity(), 0.001f))
    {
      JPH::RotatedTranslatedShapeSettings outerShape;
      outerShape.mInnerShapePtr = pSubShape;
      outerShape.mPosition = WJoltConversionUtils::ToVec3(shapeTransform.m_vPosition);
      outerShape.mRotation = WJoltConversionUtils::ToQuat(shapeTransform.m_qRotation);
      outerShape.mUserData = reinterpret_cast<WUInt64>(m_pJoltUserData);

      pBodyDesc->SetShape(outerShape.Create().Get());
    }
    else
    {
      pBodyDesc->SetShape(pSubShape);
    }

    pSubShape->Release(); // had to manual AddRef once
  }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void WJoltRagdollComponent::SetupLimbJoints(const WSkeletonResource* pSkeleton, JPH::RagdollSettings* pRagdollSettings)
{
  // TODO: still needed ? (it should be)
  // the main direction of Jolt bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  // const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // const WQuat qBoneDirAdjustment = -WBasisAxis::GetBasisRotation(srcBoneDir, WBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (WUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_uiPartIndex == WInvalidJointIndex)
      continue;

    const WSkeletonJoint& thisJoint = skeleton.GetJointByIndex(uiLimbIdx);
    WUInt16 uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != WInvalidJointIndex && m_Limbs[uiParentLimb].m_uiPartIndex == WInvalidJointIndex)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == WInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    CreateLimbJoint(thisJoint, &pRagdollSettings->mParts[parentLimb.m_uiPartIndex], &pRagdollSettings->mParts[thisLimb.m_uiPartIndex]);
  }
}

void WJoltRagdollComponent::CreateLimbJoint(const WSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc)
{
  WEnum<WSkeletonJointType> jointType = thisJoint.GetJointType();

  for (WUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == thisJoint.GetName())
    {
      jointType = m_JointOverrides[i].m_JointType;
      break;
    }
  }

  if (jointType == WSkeletonJointType::None)
    return;

  JPH::RagdollSettings::Part* pLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pThisBodyDesc);
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pParentBodyDesc);

  WTransform tParent = WJoltConversionUtils::ToTransform(pParentLink->mPosition, pParentLink->mRotation);
  WTransform tThis = WJoltConversionUtils::ToTransform(pLink->mPosition, pLink->mRotation);

  if (jointType == WSkeletonJointType::Fixed)
  {
    JPH::FixedConstraintSettings* pJoint = new JPH::FixedConstraintSettings();
    pLink->mToParent = pJoint;

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPoint1 = pLink->mPosition;
    pJoint->mPoint2 = pLink->mPosition;
  }

  if (jointType == WSkeletonJointType::SwingTwist)
  {
    JPH::SwingTwistConstraintSettings* pJoint = new JPH::SwingTwistConstraintSettings();
    pLink->mToParent = pJoint;

    const WQuat offsetRot = thisJoint.GetLocalOrientation();

    pJoint->mSpace = JPH::EConstraintSpace::WorldSpace;
    pJoint->mDrawConstraintSize = 0.15f;
    pJoint->mPosition1 = pLink->mPosition;
    pJoint->mPosition2 = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian();
    pJoint->mPlaneHalfConeAngle = thisJoint.GetHalfSwingLimitY().GetRadian();
    pJoint->mTwistMinAngle = thisJoint.GetTwistLimitLow().GetRadian();
    pJoint->mTwistMaxAngle = thisJoint.GetTwistLimitHigh().GetRadian();
    pJoint->mMaxFrictionTorque = m_fStiffnessFactor * thisJoint.GetStiffness();
    pJoint->mPlaneAxis1 = WJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * WVec3::MakeAxisZ()).Normalized();
    pJoint->mPlaneAxis2 = WJoltConversionUtils::ToVec3(tThis.m_qRotation * WVec3::MakeAxisZ()).Normalized();
    pJoint->mTwistAxis1 = WJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * WVec3::MakeAxisY()).Normalized();
    pJoint->mTwistAxis2 = WJoltConversionUtils::ToVec3(tThis.m_qRotation * WVec3::MakeAxisY()).Normalized();

    pJoint->mSwingMotorSettings.mSpringSettings.mFrequency = 20;
    pJoint->mSwingMotorSettings.mSpringSettings.mStiffness = 20;
    pJoint->mSwingMotorSettings.mSpringSettings.mDamping = 2;

    pJoint->mSwingMotorSettings.SetForceLimit(m_fMotorStrength);
    pJoint->mTwistMotorSettings.SetForceLimit(m_fMotorStrength);

    // torque is needed for the 'powered' animation mode
    pJoint->mSwingMotorSettings.SetTorqueLimit(m_fMotorStrength);
    pJoint->mTwistMotorSettings.SetTorqueLimit(m_fMotorStrength);
  }
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);

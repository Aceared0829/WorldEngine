#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSkeletonPoseMode, 1)
  W_ENUM_CONSTANTS(WSkeletonPoseMode::CustomPose, WSkeletonPoseMode::RestPose, WSkeletonPoseMode::Disabled)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WSkeletonPoseComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton"), new WRequiredAttribute()),
    W_ENUM_ACCESSOR_PROPERTY("Mode", WSkeletonPoseMode, GetPoseMode, SetPoseMode),
    W_MEMBER_PROPERTY("EditBones", m_fDummy),
    W_MAP_ACCESSOR_PROPERTY("Bones", GetBones, GetBone, SetBone, RemoveBone)->AddAttributes(new WExposedParametersAttribute("Skeleton"), new WContainerAttribute(false, true, false)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
    new WBoneManipulatorAttribute("Bones", "EditBones"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSkeletonPoseComponent::WSkeletonPoseComponent() = default;
WSkeletonPoseComponent::~WSkeletonPoseComponent() = default;

void WSkeletonPoseComponent::Update()
{
  if (m_uiResendPose == 0)
    return;

  if (--m_uiResendPose > 0)
  {
    static_cast<WSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
  }

  if (m_PoseMode == WSkeletonPoseMode::RestPose)
  {
    SendRestPose();
    return;
  }

  if (m_PoseMode == WSkeletonPoseMode::CustomPose)
  {
    SendCustomPose();
    return;
  }
}

void WSkeletonPoseComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_PoseMode;

  m_Bones.Sort();
  WUInt16 numBones = static_cast<WUInt16>(m_Bones.GetCount());
  s << numBones;

  for (WUInt16 i = 0; i < numBones; ++i)
  {
    s << m_Bones.GetKey(i);
    s << m_Bones.GetValue(i).m_sName;
    s << m_Bones.GetValue(i).m_sParent;
    s << m_Bones.GetValue(i).m_Transform;
  }
}

void WSkeletonPoseComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_PoseMode;

  WHashedString sKey;
  WExposedBone bone;

  WUInt16 numBones = 0;
  s >> numBones;
  m_Bones.Reserve(numBones);

  for (WUInt16 i = 0; i < numBones; ++i)
  {
    s >> sKey;
    s >> bone.m_sName;
    s >> bone.m_sParent;
    s >> bone.m_Transform;

    m_Bones[sKey] = bone;
  }
  ResendPose();
}

void WSkeletonPoseComponent::OnActivated()
{
  SUPER::OnActivated();

  ResendPose();
}

void WSkeletonPoseComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ResendPose();
}

void WSkeletonPoseComponent::SetSkeleton(const WSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    ResendPose();
  }
}

void WSkeletonPoseComponent::SetPoseMode(WEnum<WSkeletonPoseMode> mode)
{
  m_PoseMode = mode;
  ResendPose();
}

void WSkeletonPoseComponent::ResendPose()
{
  if (m_uiResendPose == 2)
    return;

  m_uiResendPose = 2;
  static_cast<WSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
}

const WRangeView<const char*, WUInt32> WSkeletonPoseComponent::GetBones() const
{
  return WRangeView<const char*, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_Bones.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    { return m_Bones.GetKey(uiIt).GetString().GetData(); });
}

void WSkeletonPoseComponent::SetBone(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  if (value.GetReflectedType() == WGetStaticRTTI<WExposedBone>())
  {
    m_Bones[hs] = *reinterpret_cast<const WExposedBone*>(value.GetData());
  }

  // TODO
  // if (IsActiveAndInitialized())
  //{
  //  // only add to update list, if not yet activated,
  //  // since OnActivate will do the instantiation anyway
  //  GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
  //}
  ResendPose();
}

void WSkeletonPoseComponent::RemoveBone(const char* szKey)
{
  if (m_Bones.RemoveAndCopy(WTempHashedString(szKey)))
  {
    // TODO
    // if (IsActiveAndInitialized())
    //{
    //  // only add to update list, if not yet activated,
    //  // since OnActivate will do the instantiation anyway
    //  GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
    //}

    ResendPose();
  }
}

bool WSkeletonPoseComponent::GetBone(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Bones.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value.CopyTypedObject(&m_Bones.GetValue(it), WGetStaticRTTI<WExposedBone>());
  return true;
}

void WSkeletonPoseComponent::SendRestPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  if (skel.GetJointCount() == 0)
    return;

  WTempArray<ozz::math::Float4x4> poseMatrices;
  poseMatrices.SetCountUninitialized(skel.GetJointCount());
  W_ASSERT_DEBUG(WMemoryUtils::IsAligned(poseMatrices.GetData(), alignof(ozz::math::Float4x4)), "Unaligned cast");
  {
    ozz::animation::LocalToModelJob job;
    job.input = skel.GetOzzSkeleton().joint_rest_poses();
    job.output = ozz::span<ozz::math::Float4x4>(poseMatrices.GetData(), poseMatrices.GetCount());
    job.skeleton = &skel.GetOzzSkeleton();
    job.Run();
  }

  WMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = poseMatrices.GetArrayPtr().Cast<const WMat4>();

  GetOwner()->SendMessageRecursive(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = WSkeletonPoseMode::Disabled;
}

void WSkeletonPoseComponent::SendCustomPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  WTempArray<ozz::math::Float4x4> finalTransforms;
  finalTransforms.SetCountUninitialized(skel.GetJointCount());
  W_ASSERT_DEBUG(WMemoryUtils::IsAligned(finalTransforms.GetData(), alignof(ozz::math::Float4x4)), "Unaligned cast");

  for (WUInt32 i = 0; i < finalTransforms.GetCount(); ++i)
  {
    finalTransforms[i] = ozz::math::Float4x4::identity();
  }

  ozz::vector<ozz::math::SoaTransform> ozzLocalTransforms;
  ozzLocalTransforms.resize((skel.GetJointCount() + 3) / 4);

  auto restPoses = skel.GetOzzSkeleton().joint_rest_poses();

  // initialize the skeleton with the rest pose
  for (WUInt32 i = 0; i < ozzLocalTransforms.size(); ++i)
  {
    ozzLocalTransforms[i] = restPoses[i];
  }

  for (const auto& boneIt : m_Bones)
  {
    const WUInt16 uiBone = skel.FindJointByName(boneIt.key);
    if (uiBone == WInvalidJointIndex)
      continue;

    const WExposedBone& thisBone = boneIt.value;

    // this can happen when the property was reverted
    if (thisBone.m_sName.IsEmpty() || thisBone.m_sParent.IsEmpty())
      continue;

    W_ASSERT_DEBUG(!thisBone.m_Transform.m_qRotation.IsNaN(), "Invalid bone transform in pose component");

    const WQuat& boneRot = thisBone.m_Transform.m_qRotation;

    const WUInt32 idx0 = uiBone / 4;
    const WUInt32 idx1 = uiBone % 4;

    ozz::math::SoaQuaternion& q = ozzLocalTransforms[idx0].rotation;
    reinterpret_cast<float*>(&q.x)[idx1] = boneRot.x;
    reinterpret_cast<float*>(&q.y)[idx1] = boneRot.y;
    reinterpret_cast<float*>(&q.z)[idx1] = boneRot.z;
    reinterpret_cast<float*>(&q.w)[idx1] = boneRot.w;
  }

  ozz::animation::LocalToModelJob job;
  job.input = ozz::span<const ozz::math::SoaTransform>(ozzLocalTransforms.data(), ozzLocalTransforms.size());
  job.output = ozz::span<ozz::math::Float4x4>(finalTransforms.GetData(), finalTransforms.GetCount());
  job.skeleton = &skel.GetOzzSkeleton();
  W_ASSERT_DEBUG(job.Validate(), "");
  job.Run();


  WMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = finalTransforms.GetArrayPtr().Cast<const WMat4>();

  GetOwner()->SendMessageRecursive(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = WSkeletonPoseMode::Disabled;
}

//////////////////////////////////////////////////////////////////////////

void WSkeletonPoseComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  WDeque<WComponentHandle> requireUpdate;

  {
    W_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    WSkeletonPoseComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->Update();
  }
}

void WSkeletonPoseComponentManager::EnqueueUpdate(WComponentHandle hComponent)
{
  W_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != WInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void WSkeletonPoseComponentManager::Initialize()
{
  SUPER::Initialize();

  WWorldModule::UpdateFunctionDesc desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSkeletonPoseComponentManager::Update, this);
  desc.m_Phase = WWorldUpdatePhase::PreAsync;

  RegisterUpdateFunction(desc);
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);

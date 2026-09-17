#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAnimatedMeshComponent, 13, WComponentMode::Static);
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    W_MESSAGE_HANDLER(WMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
    W_MESSAGE_HANDLER(WMsgCustomInstanceDataOffsetChanged, OnMsgCustomInstanceDataOffsetChanged),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE

W_BEGIN_STATIC_REFLECTED_ENUM(WRootMotionMode, 1)
  W_ENUM_CONSTANTS(WRootMotionMode::Ignore, WRootMotionMode::ApplyToOwner, WRootMotionMode::SendMoveCharacterMsg)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WAnimatedMeshComponent::WAnimatedMeshComponent() = default;
WAnimatedMeshComponent::~WAnimatedMeshComponent() = default;

void WAnimatedMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void WAnimatedMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  W_ASSERT_DEV(uiVersion >= 13, "Unsupported version, delete the file and reexport it");
}

void WAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void WAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void WAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = WBoundingBox::MakeInvalid();

  if (!m_hMesh.IsValid())
    return;

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  m_hDefaultSkeleton = pMesh->m_hDefaultSkeleton;
  const auto hSkeleton = m_hDefaultSkeleton;

  if (!hSkeleton.IsValid())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(hSkeleton, WResourceAcquireMode::BlockTillLoaded);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  {
    const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    const WUInt32 uiNumSkeletonJoints = pOzzSkeleton->num_joints();

    WTempArray<ozz::math::Float4x4> poseMatrices;
    poseMatrices.SetCountUninitialized(uiNumSkeletonJoints);
    W_ASSERT_DEBUG(WMemoryUtils::IsAligned(poseMatrices.GetData(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    {
      ozz::animation::LocalToModelJob job;
      job.input = pOzzSkeleton->joint_rest_poses();
      job.output = ozz::span<ozz::math::Float4x4>(poseMatrices.GetData(), poseMatrices.GetCount());
      job.skeleton = pOzzSkeleton;
      job.Run();
    }

    WMsgAnimationPoseUpdated msg;
    msg.m_ModelTransforms = poseMatrices.GetArrayPtr().Cast<const WMat4>();
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

    OnAnimationPoseUpdated(msg);
  }

  TriggerLocalBoundsUpdate();
}


void WAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const WHashTable<WHashedString, WMeshResourceDescriptor::BoneData>& bones, const WSkeleton& skeleton, WArrayPtr<const WMat4> modelSpaceTransforms, WBoundingBox* bounds)
{
  auto boneTransforms = m_SkinningState.GetOrCreateBoneTransformsForWriting(*this, bones.GetCount());

  if (bounds)
  {
    for (auto itBone : bones)
    {
      const WUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == WInvalidJointIndex)
        continue;

      bounds->ExpandToInclude(modelSpaceTransforms[uiJointIdx].GetTranslationVector());
      boneTransforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  else
  {
    for (auto itBone : bones)
    {
      const WUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == WInvalidJointIndex)
        continue;

      boneTransforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
}

WTransform WAnimatedMeshComponent::GetFinalGlobalTransform() const
{
  return GetOwner()->GetGlobalTransform() * m_RootTransform;
}

WMeshRenderData* WAnimatedMeshComponent::CreateRenderData(const WRenderDataManager* pRenderDataManager) const
{
  auto pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<WSkinnedMeshRenderData>(GetOwner());

  pRenderData->m_DataOffsets.m_uiSkinning = m_SkinningState.m_DataOffset.m_uiOffset;
  pRenderData->m_hSkinningBuffer = pRenderDataManager->GetSkinningDataBuffer();

  return pRenderData;
}

void WAnimatedMeshComponent::RetrievePose(WDynamicArray<WMat4>& out_modelTransforms, WTransform& out_rootTransform, const WSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (!m_hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);

  const WHashTable<WHashedString, WMeshResourceDescriptor::BoneData>& bones = pMesh->m_Bones;
  auto boneTransforms = m_SkinningState.GetBoneTransformsForReading();

  out_modelTransforms.SetCount(skeleton.GetJointCount(), WMat4::MakeIdentity());

  for (auto itBone : bones)
  {
    const WUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == WInvalidJointIndex)
      continue;

    out_modelTransforms[uiJointIdx] = boneTransforms[itBone.Value().m_uiBoneIndex].GetAsMat4() * itBone.Value().m_GlobalInverseRestPoseMatrix.GetInverse();
  }
}

void WAnimatedMeshComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg)
{
  if (!m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);

  WBoundingBox poseBounds;
  poseBounds = WBoundingBox::MakeInvalid();
  MapModelSpacePoseToSkinningSpace(pMesh->m_Bones, *msg.m_pSkeleton, msg.m_ModelTransforms, &poseBounds);

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

void WAnimatedMeshComponent::OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg)
{
  if (!msg.m_hSkeleton.IsValid() && m_hMesh.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the WSkeletonComponent)

    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

void WAnimatedMeshComponent::OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& msg)
{
  m_SkinningState.m_DataOffset = msg.m_NewOffset;

  InvalidateCachedRenderData();
}

WResult WAnimatedMeshComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  if (!m_MaxBounds.IsValid() || !m_hMesh.IsValid())
    return W_FAILURE;

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  WBoundingBox bbox = m_MaxBounds;
  bbox.Grow(WVec3(pMesh->m_fMaxBoneVertexOffset));
  bounds = WBoundingBoxSphere::MakeFromBox(bbox);
  bounds.Transform(m_RootTransform.GetAsMat4());
  return W_SUCCESS;
}

void WRootMotionMode::Apply(WRootMotionMode::Enum mode, WGameObject* pObject, const WVec3& vTranslation, WAngle rotationX, WAngle rotationY, WAngle rotationZ)
{
  switch (mode)
  {
    case WRootMotionMode::Ignore:
      return;

    case WRootMotionMode::ApplyToOwner:
    {
      WVec3 vNewPos = pObject->GetLocalPosition();
      vNewPos += pObject->GetLocalRotation() * vTranslation;
      pObject->SetLocalPosition(vNewPos);

      // not tested whether this is actually correct
      WQuat rotation = WQuat::MakeFromEulerAngles(rotationX, rotationY, rotationZ);

      pObject->SetLocalRotation(rotation * pObject->GetLocalRotation());

      return;
    }

    case WRootMotionMode::SendMoveCharacterMsg:
    {
      WMsgApplyRootMotion msg;
      msg.m_vTranslation = vTranslation;
      msg.m_RotationX = rotationX;
      msg.m_RotationY = rotationY;
      msg.m_RotationZ = rotationZ;

      while (pObject != nullptr)
      {
        pObject->SendMessage(msg);
        pObject = pObject->GetParent();
      }

      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////


WAnimatedMeshComponentManager::WAnimatedMeshComponentManager(WWorld* pWorld)
  : WComponentManager<ComponentType, WBlockStorageType::FreeList>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WAnimatedMeshComponentManager::ResourceEventHandler, this));
}

WAnimatedMeshComponentManager::~WAnimatedMeshComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void WAnimatedMeshComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WAnimatedMeshComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading)
  {
    if (WMeshResource* pResource = WDynamicCast<WMeshResource*>(e.m_pResource))
    {
      WMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hMesh == hMesh)
        {
          AddToUpdateList(it);
        }
      }
    }

    if (WSkeletonResource* pResource = WDynamicCast<WSkeletonResource*>(e.m_pResource))
    {
      WSkeletonResourceHandle hSkeleton(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hDefaultSkeleton == hSkeleton)
        {
          AddToUpdateList(it);
        }
      }
    }
  }
}

void WAnimatedMeshComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    WAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void WAnimatedMeshComponentManager::AddToUpdateList(WAnimatedMeshComponent* pComponent)
{
  WComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == WInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);

#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/LodAnimatedMeshComponent.h>
#include <RendererCore/Components/LodComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WLodAnimatedMeshLod, WNoBase, 2, WRTTIDefaultAllocator<WLodAnimatedMeshLod>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Threshold", m_fThreshold)
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WLodAnimatedMeshComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
    W_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    W_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ARRAY_MEMBER_PROPERTY("Meshes", m_Meshes),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
    new WSphereVisualizerAttribute("BoundsRadius", WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "BoundsOffset"),
    new WTransformManipulatorAttribute("BoundsOffset"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetCustomData, OnMsgSetCustomData),
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    W_MESSAGE_HANDLER(WMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

struct LodAnimatedMeshCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

WLodAnimatedMeshComponent::WLodAnimatedMeshComponent() = default;
WLodAnimatedMeshComponent::~WLodAnimatedMeshComponent() = default;

void WLodAnimatedMeshComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo, bShow);
}

bool WLodAnimatedMeshComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo);
}

void WLodAnimatedMeshComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges, bShow);
}

bool WLodAnimatedMeshComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges);
}

void WLodAnimatedMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_Meshes.GetCount();
  for (const auto& mesh : m_Meshes)
  {
    s << mesh.m_hMesh;
    s << mesh.m_fThreshold;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s << m_vCustomData;
}

void WLodAnimatedMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  WUInt32 uiMeshes = 0;
  s >> uiMeshes;

  m_Meshes.SetCount(uiMeshes);

  for (auto& mesh : m_Meshes)
  {
    s >> mesh.m_hMesh;
    s >> mesh.m_fThreshold;
  }

  s >> m_Color;
  s >> m_fSortingDepthOffset;

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  if (uiVersion >= 2)
  {
    s >> m_vCustomData;
  }
}

WResult WLodAnimatedMeshComponent::GetLocalBounds(WBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = WBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return W_SUCCESS;
}

void WLodAnimatedMeshComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (m_Meshes.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::EditorView || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::MainView)
  {
    UpdateSelectedLod(*msg.m_pView);
  }

  if (m_iCurLod >= (WInt32)m_Meshes.GetCount())
    return;

  auto hMesh = m_Meshes[m_iCurLod].m_hMesh;

  if (!hMesh.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  const WTransform finalTransform = GetOwner()->GetGlobalTransform() * m_RootTransform;
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, finalTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (WUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const WUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    WMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    WSkinnedMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WSkinnedMeshRenderData>(GetOwner());
    {
      // Already done in CreateRenderDataForThisFrame but only with the owner's transform. We need to use the final transform here.
      pRenderData->m_vGlobalPosition = finalTransform.m_vPosition;
      pRenderData->m_Flags.AddOrRemove(WRenderData::Flags::FlipWinding, finalTransform.HasMirrorScaling());

      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_DataOffsets.m_uiSkinning = m_SkinningState.m_DataOffset.m_uiOffset;
      pRenderData->m_hSkinningBuffer = msg.m_pRenderDataManager->GetSkinningDataBuffer();

      pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, hMesh, uiMaterialIndex, uiPartIndex);
    }

    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial);

    msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);
  }
}

void WLodAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const WHashTable<WHashedString, WMeshResourceDescriptor::BoneData>& bones, const WSkeleton& skeleton, WArrayPtr<const WMat4> modelSpaceTransforms, WBoundingBox* bounds)
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

void WLodAnimatedMeshComponent::SetColor(const WColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const WColor& WLodAnimatedMeshComponent::GetColor() const
{
  return m_Color;
}

void WLodAnimatedMeshComponent::SetCustomData(const WVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const WVec4& WLodAnimatedMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void WLodAnimatedMeshComponent::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float WLodAnimatedMeshComponent::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void WLodAnimatedMeshComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void WLodAnimatedMeshComponent::OnMsgSetCustomData(WMsgSetCustomData& ref_msg)
{
  m_vCustomData = ref_msg.m_vData;

  InvalidateCachedRenderData();
}

void WLodAnimatedMeshComponent::RetrievePose(WDynamicArray<WMat4>& out_modelTransforms, WTransform& out_rootTransform, const WSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (m_Meshes.IsEmpty())
    return;

  auto hMesh = m_Meshes[0].m_hMesh;

  if (!hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::BlockTillLoaded);

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

static float CalculateSphereScreenSpaceCoverage(const WBoundingSphere& sphere, const WCamera& camera)
{
  if (camera.IsPerspective())
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

void WLodAnimatedMeshComponent::UpdateSelectedLod(const WView& view) const
{
  const WInt32 iNumLods = (WInt32)m_Meshes.GetCount();

  const WVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = WMath::Max(vScale.x, vScale.y, vScale.z);
  const WVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(WBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *view.GetLodCamera()) * WMath::Max(0.0f, (float)cvar_RenderingLodCoverageScale);

  // clamp the input value, this is to prevent issues while editing the threshold array
  WInt32 iNewLod = WMath::Clamp<WInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_Meshes[iNewLod - 1].m_fThreshold;
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_Meshes[iNewLod].m_fThreshold;
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_Meshes[iNewLod + 1].m_fThreshold);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = WMath::Clamp(iNewLod, 0, iNumLods);

  if (cvar_RenderingLodForce >= 0)
  {
    iNewLod = WMath::Min<WInt32>(cvar_RenderingLodForce, iNumLods - 1);
  }

  m_iCurLod = iNewLod;

  if (GetShowDebugInfo())
  {
    WStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", WArgF(fCoverage, 3), iNewLod, WArgF(fCoverageP, 3), WArgF(fCoverageN, 3));
    WDebugRenderer::Draw3DText(view.GetHandle(), sb, GetOwner()->GetGlobalPosition(), WColor::White);
  }
}

void WLodAnimatedMeshComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  WResourceLock<WMeshResource> pMesh(m_Meshes[0].m_hMesh, WResourceAcquireMode::BlockTillLoaded);

  WBoundingBox poseBounds;
  poseBounds = WBoundingBox::MakeInvalid();
  MapModelSpacePoseToSkinningSpace(pMesh->m_Bones, *msg.m_pSkeleton, msg.m_ModelTransforms, &poseBounds);

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((WRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (W_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }
}

void WLodAnimatedMeshComponent::OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  if (!msg.m_hSkeleton.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the WSkeletonComponent)

    WResourceLock<WMeshResource> pMesh(m_Meshes[0].m_hMesh, WResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

void WLodAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void WLodAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

void WLodAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = WBoundingBox::MakeInvalid();

  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  WResourceLock<WMeshResource> pMesh(m_Meshes[0].m_hMesh, WResourceAcquireMode::BlockTillLoaded);
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

//////////////////////////////////////////////////////////////////////////


WLodAnimatedMeshComponentManager::WLodAnimatedMeshComponentManager(WWorld* pWorld)
  : WComponentManager<ComponentType, WBlockStorageType::FreeList>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

WLodAnimatedMeshComponentManager::~WLodAnimatedMeshComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void WLodAnimatedMeshComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WLodAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WLodAnimatedMeshComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading)
  {
    if (WMeshResource* pResource = WDynamicCast<WMeshResource*>(e.m_pResource))
    {
      WMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        for (auto& am : it->m_Meshes)
        {
          if (am.m_hMesh == hMesh)
          {
            AddToUpdateList(it);
          }
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

void WLodAnimatedMeshComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    WLodAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void WLodAnimatedMeshComponentManager::AddToUpdateList(WLodAnimatedMeshComponent* pComponent)
{
  WComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == WInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_LodAnimatedMeshComponent);

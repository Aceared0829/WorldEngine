#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipContext, 1, WRTTIDefaultAllocator<WAnimationClipContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Animation Clip"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimationClipContext::WAnimationClipContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WAnimationClipContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = WDynamicCast<const WQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = WDynamicCast<const WSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      W_LOCK(pWorld->GetWriteMarker());

      WAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        pAnimMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        m_hAnimMeshComponent = WAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewAnim" && m_sBaseAnimationClip != pMsg->m_sPayload)
    {
      m_sBaseAnimationClip = pMsg->m_sPayload;
    }
    else if (pMsg->m_sWhatToDo == "PlaybackPos")
    {
      SetPlaybackPosition(pMsg->m_PayloadValue.Get<double>());
    }
    else if (pMsg->m_sWhatToDo == "ExtractRootMotionFromFeet")
    {
      ExtractRootMotionFromFeet();
    }

    return;
  }

  if (auto pMsg = WDynamicCast<const WViewRedrawMsgToEngine*>(pMsg0))
  {
    auto pWorld = m_pWorld;
    W_LOCK(pWorld->GetWriteMarker());

    if (!m_sAnimatedMeshToUse.IsEmpty())
    {
      WStringBuilder sAnimClipGuid;
      WConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
      WAnimationClipResourceHandle hAnimation = WResourceManager::LoadResource<WAnimationClipResource>(sAnimClipGuid);

      WResourceLock<WAnimationClipResource> pAnimation(hAnimation, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
      if (pAnimation.GetAcquireResult() == WResourceAcquireResult::Final)
      {
        WSimpleDocumentConfigMsgToEditor msg;
        msg.m_DocumentGuid = pMsg->m_DocumentGuid;
        msg.m_sWhatToDo = "ClipDuration";
        msg.m_PayloadValue = pAnimation->GetDescriptor().GetDuration();
        SendProcessMessage(&msg);
      }
    }

    GenerateAndApplyPose();
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void WAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
  }
}

WEngineProcessViewContext* WAnimationClipContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WAnimationClipViewContext, this);
}

void WAnimationClipContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WAnimationClipContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_fNormalizedPlaybackPosition = 0.5f;
    GenerateAndApplyPose();

    m_pWorld->SetWorldSimulationEnabled(true);
    m_pWorld->Update();
    m_pWorld->SetWorldSimulationEnabled(false);
  }

  WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WAnimationClipViewContext* pMeshViewContext = static_cast<WAnimationClipViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WAnimationClipContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const WQuerySelectionBBoxMsgToEngine* msg = static_cast<const WQuerySelectionBBoxMsgToEngine*>(pMsg);

  WQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void WAnimationClipContext::SetPlaybackPosition(double pos)
{
  m_fNormalizedPlaybackPosition = static_cast<float>(pos);
}

void WAnimationClipContext::GenerateAndApplyPose()
{
  if (m_sAnimatedMeshToUse.IsEmpty() || m_pGameObject == nullptr)
    return;

  WMeshResourceHandle hAnimMesh = WResourceManager::LoadResource<WMeshResource>(m_sAnimatedMeshToUse);
  WResourceLock<WMeshResource> pAnimMesh(hAnimMesh, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pAnimMesh.GetAcquireResult() != WResourceAcquireResult::Final || !pAnimMesh->m_hDefaultSkeleton.IsValid())
    return;

  WResourceLock<WSkeletonResource> pSkeleton(pAnimMesh->m_hDefaultSkeleton, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WStringBuilder sAnimClipGuid;
  WConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
  WAnimationClipResourceHandle hAnimation = WResourceManager::LoadResource<WAnimationClipResource>(sAnimClipGuid);

  WResourceLock<WAnimationClipResource> pAnimation(hAnimation, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pAnimation.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer(), m_pGameObject);

  bool bGraphSetup = false;

  if (!m_sBaseAnimationClip.IsEmpty())
  {
    // Additive mode: blend base animation with the additive clip on top.
    // The CombinePoses command automatically separates additive and non-additive
    // layers based on the m_bAdditive flag in each clip's descriptor.
    WAnimationClipResourceHandle hBaseAnim = WResourceManager::LoadResource<WAnimationClipResource>(m_sBaseAnimationClip);
    WResourceLock<WAnimationClipResource> pBaseAnim(hBaseAnim, WResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pBaseAnim.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      auto& cmdBase = poseGen.AllocCommandSampleTrack(0);
      cmdBase.m_hAnimationClip = hBaseAnim;
      cmdBase.m_fNormalizedSamplePos = 0.0f;
      cmdBase.m_fPreviousNormalizedSamplePos = 0.0f;
      cmdBase.m_EventSampling = WAnimPoseEventTrackSampleMode::None;

      auto& cmdAdditive = poseGen.AllocCommandSampleTrack(1);
      cmdAdditive.m_hAnimationClip = hAnimation;
      cmdAdditive.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
      cmdAdditive.m_fPreviousNormalizedSamplePos = m_fNormalizedPlaybackPosition;
      cmdAdditive.m_EventSampling = WAnimPoseEventTrackSampleMode::None;

      auto& cmdCombine = poseGen.AllocCommandCombinePoses();
      cmdCombine.m_Inputs.PushBack(cmdBase.GetCommandID());
      cmdCombine.m_InputWeights.PushBack(1.0f);
      cmdCombine.m_Inputs.PushBack(cmdAdditive.GetCommandID());
      cmdCombine.m_InputWeights.PushBack(1.0f);

      auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
      cmdL2M.m_pSendLocalPoseMsgTo = m_pGameObject;
      cmdL2M.m_Inputs.PushBack(cmdCombine.GetCommandID());
      poseGen.SetFinalCommand(cmdL2M.GetCommandID());

      bGraphSetup = true;
    }
  }

  if (!bGraphSetup)
  {
    // Non-additive mode (or base clip not yet loaded): sample the clip directly.
    auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
    cmdSample.m_hAnimationClip = hAnimation;
    cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
    cmdSample.m_fPreviousNormalizedSamplePos = m_fNormalizedPlaybackPosition;
    cmdSample.m_EventSampling = WAnimPoseEventTrackSampleMode::None;

    auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = m_pGameObject;
    cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    poseGen.SetFinalCommand(cmdL2M.GetCommandID());
  }

  poseGen.UpdatePose(false);

  if (poseGen.ShouldSendPoseResultMsg())
  {
    WMsgAnimationPoseUpdated poseMsg;
    poseMsg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    poseMsg.m_ModelTransforms = poseGen.GetCurrentPose();
    m_pGameObject->SendMessageRecursive(poseMsg);
  }
}

void WAnimationClipContext::ExtractRootMotionFromFeet()
{
  auto ReturnFailure = [this](WStringView str)
  {
    WSimpleDocumentConfigMsgToEditor msg;
    msg.m_sWhatToDo = "ReportError";
    msg.m_sPayload = str;
    SendProcessMessage(&msg);
  };

  if (m_sAnimatedMeshToUse.IsEmpty())
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nPreview mesh is not set.");
    return;
  }

  WStringBuilder sAnimClipGuid;
  WConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
  WAnimationClipResourceHandle hAnimation = WResourceManager::LoadResource<WAnimationClipResource>(sAnimClipGuid);

  WResourceLock<WAnimationClipResource> pAnimation(hAnimation, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't load animation.");
    return;
  }

  WMeshResourceHandle hAnimMesh = WResourceManager::LoadResource<WMeshResource>(m_sAnimatedMeshToUse);
  WResourceLock<WMeshResource> pAnimMesh(hAnimMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pAnimMesh.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't load preview mesh.");
    return;
  }

  if (!pAnimMesh->m_hDefaultSkeleton.IsValid())
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nPreview mesh has no skeleton.");
    return;
  }

  WResourceLock<WSkeletonResource> pSkeleton(pAnimMesh->m_hDefaultSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nSkeleton of preview mesh could not be loaded.");
    return;
  }

  WAnimPoseGenerator pg;

  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  const WUInt16 uiFoot1 = pSkeleton->GetDescriptor().m_uiLeftFootJoint;
  const WUInt16 uiFoot2 = pSkeleton->GetDescriptor().m_uiRightFootJoint;

  if (uiFoot1 == WInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nLeft foot joint is not correctly defined in skeleton asset.");
    return;
  }

  if (uiFoot2 == WInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nRight foot joint is not correctly defined in skeleton asset.");
    return;
  }

  if (uiFoot1 == uiFoot2)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nLeft and right foot joint must differ.");
    return;
  }

  WUInt16 uiSharedParentJoint = WInvalidJointIndex;

  // find shared parent bone
  {
    WTempHybridArray<WUInt16, 32> parents;

    auto* pJoint = &skel.GetJointByIndex(uiFoot1);

    // collect all parent joint indices
    while (pJoint->GetParentIndex() != WInvalidJointIndex)
    {
      parents.PushBack(pJoint->GetParentIndex());
      pJoint = &skel.GetJointByIndex(pJoint->GetParentIndex());
    }

    pJoint = &skel.GetJointByIndex(uiFoot2);

    // collect all parent joint indices
    while (pJoint->GetParentIndex() != WInvalidJointIndex)
    {
      if (parents.Contains(pJoint->GetParentIndex()))
      {
        uiSharedParentJoint = pJoint->GetParentIndex();
        break;
      }

      pJoint = &skel.GetJointByIndex(pJoint->GetParentIndex());
    }
  }

  if (uiSharedParentJoint == WInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't find shared parent bone of feet bones.");
    return;
  }

  // TODO: don't hard-code num samples ?
  const WUInt32 uiNumSamples = 32;
  float fPrevPos = 0.0f;

  int iFootDown = -1;
  int iFootUp = -1;
  WVec3 vLastHipDist(0);

  WVec3 vMovement(0);
  int iSamplesTaken = 0;


  for (WUInt32 uiSample = 0; uiSample < uiNumSamples; ++uiSample)
  {
    pg.Reset(pSkeleton.GetPointer(), nullptr);

    auto& cmd = pg.AllocCommandSampleTrack(0);
    cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::None;
    cmd.m_fPreviousNormalizedSamplePos = fPrevPos;
    cmd.m_fNormalizedSamplePos = (float)uiSample / (float)(uiNumSamples - 1);
    cmd.m_hAnimationClip = hAnimation;
    fPrevPos = cmd.m_fNormalizedSamplePos;

    auto& cmdMP = pg.AllocCommandLocalToModelPose();
    cmdMP.m_Inputs.PushBack(cmd.GetCommandID());

    pg.SetFinalCommand(cmdMP.GetCommandID());

    pg.UpdatePose(false);

    const WVec3 p[3] =
      {
        pg.GetCurrentPose()[uiSharedParentJoint].GetTranslationVector(),
        pg.GetCurrentPose()[uiFoot1].GetTranslationVector(),
        pg.GetCurrentPose()[uiFoot2].GetTranslationVector()
        //
      };

    if (iFootDown == -1)
    {
      if (p[1].y < p[2].y)
      {
        iFootDown = 1;
        iFootUp = 2;
      }
      else
      {
        iFootDown = 2;
        iFootUp = 1;
      }

      vLastHipDist = p[0] - p[iFootDown];
    }
    else if (p[iFootDown].y > p[iFootUp].y)
    {
      WMath::Swap(iFootDown, iFootUp);

      vLastHipDist = p[0] - p[iFootDown];
    }
    else
    {
      const WVec3 vHipDist = p[0] - p[iFootDown];

      vMovement += vHipDist - vLastHipDist;
      iSamplesTaken++;
      vLastHipDist = vHipDist;
    }
  }

  if (iSamplesTaken == 0)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nNo valid animation samples found.");
    return;
  }

  WVec3 avg = vMovement / (float)iSamplesTaken;
  avg *= (uiNumSamples - 1);                                           // calculate the average movement over the entire clip
  avg /= pAnimation->GetDescriptor().GetDuration().AsFloatInSeconds(); // scale it to the movement per second

  // transform the motion into the desired space
  avg = pSkeleton->GetDescriptor().m_RootTransform.GetAsMat4().TransformDirection(avg);

  const float len = avg.GetLengthAndNormalize();

  WVec4 res;
  res.Set(avg.x, avg.y, avg.z, len);

  {
    WSimpleDocumentConfigMsgToEditor msg;
    msg.m_sWhatToDo = "ExtractRootMotionFromFeet";
    msg.m_PayloadValue = res;

    SendProcessMessage(&msg);
  }
}

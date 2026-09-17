#include <GameEngine/GameEnginePCH.h>

// #include <Core/Input/InputManager.h>
// #include <Core/WorldSerializer/WorldReader.h>
// #include <Core/WorldSerializer/WorldWriter.h>
// #include <GameEngine/Animation/Skeletal/MotionMatchingComponent.h>
// #include <RendererCore/AnimationSystem/AnimationClipResource.h>
// #include <RendererCore/AnimationSystem/SkeletonResource.h>
// #include <RendererCore/Debug/DebugRenderer.h>
// #include <RendererFoundation/Device/Device.h>
//
//// clang-format off
// W_BEGIN_COMPONENT_TYPE(WMotionMatchingComponent, 2, WComponentMode::Dynamic);
//{
//   W_BEGIN_PROPERTIES
//   {
//     W_ARRAY_ACCESSOR_PROPERTY("Animations", Animations_GetCount, Animations_GetValue, Animations_SetValue, Animations_Insert, Animations_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
//   }
//   W_END_PROPERTIES;
//
//   W_BEGIN_ATTRIBUTES
//   {
//       new WCategoryAttribute("Animation"),
//   }
//   W_END_ATTRIBUTES;
// }
// W_END_COMPONENT_TYPE
//// clang-format on
//
// WMotionMatchingComponent::WMotionMatchingComponent() = default;
// WMotionMatchingComponent::~WMotionMatchingComponent() = default;
//
// void WMotionMatchingComponent::SerializeComponent(WWorldWriter& stream) const
//{
//  SUPER::SerializeComponent(stream);
//  auto& s = stream.GetStream();
//
//  s.WriteArray(m_Animations);
//}
//
// void WMotionMatchingComponent::DeserializeComponent(WWorldReader& stream)
//{
//  SUPER::DeserializeComponent(stream);
//  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
//  auto& s = stream.GetStream();
//
//  if (uiVersion >= 2)
//  {
//    s.ReadArray(m_Animations);
//  }
//}
//
// void WMotionMatchingComponent::OnSimulationStarted()
//{
//  SUPER::OnSimulationStarted();
//
//  // make sure the skinning buffer is deleted
//  W_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");
//
//  if (m_hMesh.IsValid())
//  {
//    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
//    m_hSkeleton = pMesh->GetSkeleton();
//  }
//
//  if (m_hSkeleton.IsValid())
//  {
//    WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded);
//
//    const WSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//    m_AnimationPose.Configure(skeleton);
//    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//    // m_SkinningMatrices = m_AnimationPose.GetAllTransforms();
//
//    // Create the buffer for the skinning matrices
//    WGALBufferCreationDescription BufferDesc;
//    BufferDesc.m_uiStructSize = sizeof(WMat4);
//    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
//    BufferDesc.m_bUseAsStructuredBuffer = true;
//    BufferDesc.m_bAllowShaderResourceView = true;
//    BufferDesc.m_ResourceAccess.m_bImmutable = false;
//
//    m_hSkinningTransformsBuffer = WGALDevice::GetDefaultDevice()->CreateBuffer(
//      BufferDesc, WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
//  }
//
//  // m_AnimationClipSampler.RestartAnimation();
//
//  if (m_Animations.IsEmpty())
//    return;
//
//  m_Keyframe0.m_uiAnimClip = 0;
//  m_Keyframe0.m_uiKeyframe = 0;
//  m_Keyframe1.m_uiAnimClip = 0;
//  m_Keyframe1.m_uiKeyframe = 1;
//
//  for (WUInt32 anim = 0; anim < m_Animations.GetCount(); ++anim)
//  {
//    WResourceLock<WAnimationClipResource> pClip(m_Animations[anim], WResourceAcquireMode::BlockTillLoaded);
//    WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::AllowLoadingFallback);
//
//    PrecomputeMotion(m_MotionData, "Bip01_L_Foot", "Bip01_R_Foot", pClip->GetDescriptor(), anim, pSkeleton->GetDescriptor().m_Skeleton);
//  }
//
//  m_vLeftFootPos.SetZero();
//  m_vRightFootPos.SetZero();
//
//  ConfigureInput();
//}
// void WMotionMatchingComponent::ConfigureInput()
//{
//  WInputActionConfig iac;
//  iac.m_bApplyTimeScaling = false;
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_LeftStick_PosY;
//  iac.m_sInputSlotTrigger[1] = WInputSlot_KeyUp;
//  WInputManager::SetInputActionConfig("mm", "forward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_LeftStick_NegY;
//  iac.m_sInputSlotTrigger[1] = WInputSlot_KeyDown;
//  WInputManager::SetInputActionConfig("mm", "backward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_LeftStick_NegX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  WInputManager::SetInputActionConfig("mm", "left", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_LeftStick_PosX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  WInputManager::SetInputActionConfig("mm", "right", iac, true);
//
//  iac.m_bApplyTimeScaling = true;
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_RightStick_PosX;
//  iac.m_sInputSlotTrigger[1] = WInputSlot_KeyRight;
//  // iac.m_sInputSlotTrigger[1] = WInputSlot_KeyRight;
//  WInputManager::SetInputActionConfig("mm", "turnright", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = WInputSlot_Controller0_RightStick_NegX;
//  iac.m_sInputSlotTrigger[1] = WInputSlot_KeyLeft;
//  // iac.m_sInputSlotTrigger[1] = WInputSlot_KeyRight;
//  WInputManager::SetInputActionConfig("mm", "turnleft", iac, true);
//}
//
// WVec3 WMotionMatchingComponent::GetInputDirection() const
//{
//  float fw, bw, l, r;
//
//  WInputManager::GetInputActionState("mm", "forward", &fw);
//  WInputManager::GetInputActionState("mm", "backward", &bw);
//  WInputManager::GetInputActionState("mm", "left", &l);
//  WInputManager::GetInputActionState("mm", "right", &r);
//
//  WVec3 dir;
//  dir.y = -(fw - bw);
//  dir.x = r - l;
//  dir.z = 0;
//
//  // dir.NormalizeIfNotZero(WVec3::MakeZero());
//  return dir * 3.0f;
//}
//
// WQuat WMotionMatchingComponent::GetInputRotation() const
//{
//  float tl, tr;
//
//  WInputManager::GetInputActionState("mm", "turnleft", &tl);
//  WInputManager::GetInputActionState("mm", "turnright", &tr);
//
//  const WAngle turn = WAngle::MakeFromDegree((tr - tl) * 90.0f);
//
//  WQuat q;
//  q = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), turn);
//  return q;
//}
//
// void WMotionMatchingComponent::Update()
//{
//  if (!m_hSkeleton.IsValid() || m_Animations.IsEmpty())
//    return;
//
//  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::AllowLoadingFallback);
//  const WSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//
//  // WTransform rootMotion;
//  // rootMotion.SetIdentity();
//
//  const float fKeyframeFraction = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * 24.0f; // assuming 24 FPS in the animations
//
//  {
//    const WVec3 vTargetDir = GetInputDirection() / GetOwner()->GetGlobalScaling().x;
//
//    WStringBuilder tmp;
//    tmp.SetFormat("Gamepad: {0} / {1}", WArgF(vTargetDir.x, 1), WArgF(vTargetDir.y, 1));
//    WDebugRenderer::DrawInfoText(GetWorld(), tmp, WVec2I32(10, 10), WColor::White);
//
//    m_fKeyframeLerp += fKeyframeFraction;
//    while (m_fKeyframeLerp > 1.0f)
//    {
//
//      m_Keyframe0 = m_Keyframe1;
//      m_Keyframe1 = FindNextKeyframe(m_Keyframe1, vTargetDir);
//
//      // WLog::Info("Old KF: {0} | {1} - {2}", m_Keyframe0.m_uiAnimClip, m_Keyframe0.m_uiKeyframe, m_fKeyframeLerp);
//      m_fKeyframeLerp -= 1.0f;
//      // WLog::Info("New KF: {0} | {1} - {2}", m_Keyframe1.m_uiAnimClip, m_Keyframe1.m_uiKeyframe, m_fKeyframeLerp);
//    }
//  }
//
//  m_AnimationPose.SetToBindPoseInLocalSpace(skeleton);
//
//  {
//    WResourceLock<WAnimationClipResource> pAnimClip0(m_Animations[m_Keyframe0.m_uiAnimClip], WResourceAcquireMode::BlockTillLoaded);
//    WResourceLock<WAnimationClipResource> pAnimClip1(m_Animations[m_Keyframe1.m_uiAnimClip], WResourceAcquireMode::BlockTillLoaded);
//
//    const auto& animDesc0 = pAnimClip0->GetDescriptor();
//    const auto& animDesc1 = pAnimClip1->GetDescriptor();
//
//    const auto& animatedJoints0 = animDesc0.GetAllJointIndices();
//
//    for (WUInt32 b = 0; b < animatedJoints0.GetCount(); ++b)
//    {
//      const WHashedString sJointName = animatedJoints0.GetKey(b);
//      const WUInt32 uiAnimJointIdx0 = animatedJoints0.GetValue(b);
//      const WUInt32 uiAnimJointIdx1 = animDesc1.FindJointIndexByName(sJointName);
//
//      const WUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
//      if (uiSkeletonJointIdx != WInvalidJointIndex)
//      {
//        WArrayPtr<const WTransform> pTransforms0 = animDesc0.GetJointKeyframes(uiAnimJointIdx0);
//        WArrayPtr<const WTransform> pTransforms1 = animDesc1.GetJointKeyframes(uiAnimJointIdx1);
//
//        const WTransform jointTransform1 = pTransforms0[m_Keyframe0.m_uiKeyframe];
//        const WTransform jointTransform2 = pTransforms1[m_Keyframe1.m_uiKeyframe];
//
//        WTransform res;
//        res.m_vPosition = WMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, m_fKeyframeLerp);
//        res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, m_fKeyframeLerp);
//        res.m_vScale = WMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, m_fKeyframeLerp);
//
//        m_AnimationPose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
//      }
//    }
//
//    // root motion
//    {
//      auto* pOwner = GetOwner();
//
//      WVec3 vRootMotion0, vRootMotion1;
//      vRootMotion0.SetZero();
//      vRootMotion1.SetZero();
//
//      if (animDesc0.HasRootMotion())
//        vRootMotion0 = animDesc0.GetJointKeyframes(animDesc0.GetRootMotionJoint())[m_Keyframe0.m_uiKeyframe].m_vPosition;
//      if (animDesc1.HasRootMotion())
//        vRootMotion1 = animDesc1.GetJointKeyframes(animDesc1.GetRootMotionJoint())[m_Keyframe1.m_uiKeyframe].m_vPosition;
//
//      const WVec3 vRootMotion = WMath::Lerp(vRootMotion0, vRootMotion1, m_fKeyframeLerp) * fKeyframeFraction * pOwner->GetGlobalScaling().x;
//
//      const WQuat qRotate = GetInputRotation();
//
//      const WQuat qOldRot = pOwner->GetLocalRotation();
//      const WVec3 vNewPos = qOldRot * vRootMotion + pOwner->GetLocalPosition();
//      const WQuat qNewRot = qRotate * qOldRot;
//
//      pOwner->SetLocalPosition(vNewPos);
//      pOwner->SetLocalRotation(qNewRot);
//    }
//  }
//
//  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//  const WUInt16 uiLeftFootJoint = skeleton.FindJointByName("Bip01_L_Foot");
//  const WUInt16 uiRightFootJoint = skeleton.FindJointByName("Bip01_R_Foot");
//  if (uiLeftFootJoint != WInvalidJointIndex && uiRightFootJoint != WInvalidJointIndex)
//  {
//    WTransform tLeft, tRight;
//    WBoundingSphere sphere(WVec3::MakeZero(), 0.5f);
//
//    tLeft.SetFromMat4(m_AnimationPose.GetTransform(uiLeftFootJoint));
//    tRight.SetFromMat4(m_AnimationPose.GetTransform(uiRightFootJoint));
//
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiLeftFootJoint);
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiRightFootJoint);
//
//    // const float fScaleToPerSec = (float)(1.0 / GetWorld()->GetClock().GetTimeDiff().GetSeconds());
//
//    // const WVec3 vLeftFootVel = (tLeft.m_vPosition - m_vLeftFootPos) * fScaleToPerSec;
//    // const WVec3 vRightFootVel = (tRight.m_vPosition - m_vRightFootPos) * fScaleToPerSec;
//
//    m_vLeftFootPos = tLeft.m_vPosition;
//    m_vRightFootPos = tRight.m_vPosition;
//  }
//
//  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//  WArrayPtr<WMat4> pRenderMatrices = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WMat4, m_AnimationPose.GetTransformCount());
//  WMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());
//
//  m_SkinningMatrices = pRenderMatrices;
//}
//
// void WMotionMatchingComponent::SetAnimation(WUInt32 uiIndex, const WAnimationClipResourceHandle& hResource)
//{
//  m_Animations.EnsureCount(uiIndex + 1);
//
//  m_Animations[uiIndex] = hResource;
//}
//
// WAnimationClipResourceHandle WMotionMatchingComponent::GetAnimation(WUInt32 uiIndex) const
//{
//  if (uiIndex >= m_Animations.GetCount())
//    return WAnimationClipResourceHandle();
//
//  return m_Animations[uiIndex];
//}
//
// WUInt32 WMotionMatchingComponent::Animations_GetCount() const
//{
//  return m_Animations.GetCount();
//}
//
// const char* WMotionMatchingComponent::Animations_GetValue(WUInt32 uiIndex) const
//{
//  const auto& hMat = GetAnimation(uiIndex);
//
//  if (!hMat.IsValid())
//    return "";
//
//  return hMat.GetResourceID();
//}
//
// void WMotionMatchingComponent::Animations_SetValue(WUInt32 uiIndex, const char* value)
//{
//  if (WStringUtils::IsNullOrEmpty(value))
//    SetAnimation(uiIndex, WAnimationClipResourceHandle());
//  else
//  {
//    auto hMat = WResourceManager::LoadResource<WAnimationClipResource>(value);
//    SetAnimation(uiIndex, hMat);
//  }
//}
//
// void WMotionMatchingComponent::Animations_Insert(WUInt32 uiIndex, const char* value)
//{
//  WAnimationClipResourceHandle hMat;
//
//  if (!WStringUtils::IsNullOrEmpty(value))
//    hMat = WResourceManager::LoadResource<WAnimationClipResource>(value);
//
//  m_Animations.Insert(hMat, uiIndex);
//}
//
// void WMotionMatchingComponent::Animations_Remove(WUInt32 uiIndex)
//{
//  m_Animations.RemoveAtAndCopy(uiIndex);
//}
//
// WMotionMatchingComponent::TargetKeyframe WMotionMatchingComponent::FindNextKeyframe(const TargetKeyframe& current, const WVec3& vTargetDir) const
//{
//  TargetKeyframe kf;
//  kf.m_uiAnimClip = current.m_uiAnimClip;
//  kf.m_uiKeyframe = current.m_uiKeyframe + 1;
//
//  {
//    // WResourceLock<WAnimationClipResource> pAnimClipCur(m_Animations[current.m_uiAnimClip], WResourceAcquireMode::NoFallback);
//    // const auto& animClip = pAnimClipCur->GetDescriptor();
//
//    // const WUInt32 uiLeftFootJoint = animClip.FindJointIndexByName("Bip01_L_Foot");
//    // const WUInt32 uiRightFootJoint = animClip.FindJointIndexByName("Bip01_R_Foot");
//
//    const WVec3 vLeftFootPos = m_vLeftFootPos;   // animClip.GetJointKeyframes(uiLeftFootJoint)[current.m_uiKeyframe].m_vPosition;
//    const WVec3 vRightFootPos = m_vRightFootPos; // animClip.GetJointKeyframes(uiRightFootJoint)[current.m_uiKeyframe].m_vPosition;
//
//    const WUInt32 uiBestMM = FindBestKeyframe(current, vLeftFootPos, vRightFootPos, vTargetDir);
//
//    TargetKeyframe nkf;
//    nkf.m_uiAnimClip = m_MotionData[uiBestMM].m_uiAnimClipIndex;
//    nkf.m_uiKeyframe = m_MotionData[uiBestMM].m_uiKeyframeIndex;
//
//    if ((nkf.m_uiAnimClip != kf.m_uiAnimClip) || (nkf.m_uiKeyframe != kf.m_uiKeyframe && nkf.m_uiKeyframe != current.m_uiKeyframe))
//    {
//      kf = nkf;
//    }
//  }
//
//  WResourceLock<WAnimationClipResource> pAnimClip(m_Animations[kf.m_uiAnimClip], WResourceAcquireMode::BlockTillLoaded);
//
//  if (kf.m_uiKeyframe >= pAnimClip->GetDescriptor().GetNumFrames())
//  {
//    // loop
//    kf.m_uiKeyframe = 0;
//  }
//
//  return kf;
//}
//
// void WMotionMatchingComponent::PrecomputeMotion(WDynamicArray<MotionData>& motionData, WTempHashedString jointName1, WTempHashedString jointName2,
//  const WAnimationClipResourceDescriptor& animClip, WUInt16 uiAnimClipIndex, const WSkeleton& skeleton)
//{
//  const WUInt16 uiRootJoint = animClip.HasRootMotion() ? animClip.GetRootMotionJoint() : 0xFFFFu;
//  // const WUInt16 uiJoint1IndexInAnim = animClip.FindJointIndexByName(jointName1);
//  // const WUInt16 uiJoint2IndexInAnim = animClip.FindJointIndexByName(jointName2);
//
//  const WUInt16 uiJoint1IndexInSkeleton = skeleton.FindJointByName(jointName1);
//  const WUInt16 uiJoint2IndexInSkeleton = skeleton.FindJointByName(jointName2);
//  if (uiJoint1IndexInSkeleton == WInvalidJointIndex || uiJoint2IndexInSkeleton == WInvalidJointIndex)
//    return;
//
//  const auto& jointNamesToIndices = animClip.GetAllJointIndices();
//
//  const WUInt32 uiFirstMotionDataIdx = motionData.GetCount();
//  motionData.Reserve(uiFirstMotionDataIdx + animClip.GetNumFrames());
//
//  const float fRootMotionToVelocity = animClip.GetFramesPerSecond();
//
//  WAnimationPose pose;
//  pose.Configure(skeleton);
//
//  for (WUInt16 uiFrameIdx = 0; uiFrameIdx < animClip.GetNumFrames(); ++uiFrameIdx)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//
//    for (WUInt32 b = 0; b < jointNamesToIndices.GetCount(); ++b)
//    {
//      const WUInt16 uiJointIndexInPose = skeleton.FindJointByName(jointNamesToIndices.GetKey(b));
//      if (uiJointIndexInPose != WInvalidJointIndex)
//      {
//        const WTransform jointTransform = animClip.GetJointKeyframes(jointNamesToIndices.GetValue(b))[uiFrameIdx];
//
//        pose.SetTransform(uiJointIndexInPose, jointTransform.GetAsMat4());
//      }
//    }
//
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    MotionData& md = motionData.ExpandAndGetRef();
//    md.m_vLeftFootPosition = pose.GetTransform(uiJoint1IndexInSkeleton).GetTranslationVector();
//    md.m_vRightFootPosition = pose.GetTransform(uiJoint2IndexInSkeleton).GetTranslationVector();
//    md.m_uiAnimClipIndex = uiAnimClipIndex;
//    md.m_uiKeyframeIndex = uiFrameIdx;
//    md.m_vLeftFootVelocity.SetZero();
//    md.m_vRightFootVelocity.SetZero();
//    md.m_vRootVelocity =
//      animClip.HasRootMotion() ? fRootMotionToVelocity * animClip.GetJointKeyframes(uiRootJoint)[uiFrameIdx].m_vPosition : WVec3::MakeZero();
//  }
//
//  // now compute the velocity
//  {
//    const float fScaleToVelPerSec = animClip.GetFramesPerSecond();
//
//    WUInt32 uiPrevMdIdx = motionData.GetCount() - 1;
//
//    for (WUInt32 uiMotionDataIdx = uiFirstMotionDataIdx; uiMotionDataIdx < motionData.GetCount(); ++uiMotionDataIdx)
//    {
//      {
//        WVec3 vel = motionData[uiMotionDataIdx].m_vLeftFootPosition - motionData[uiPrevMdIdx].m_vLeftFootPosition;
//        motionData[uiMotionDataIdx].m_vLeftFootVelocity = vel * fScaleToVelPerSec;
//      }
//      {
//        WVec3 vel = motionData[uiMotionDataIdx].m_vRightFootPosition - motionData[uiPrevMdIdx].m_vRightFootPosition;
//        motionData[uiMotionDataIdx].m_vRightFootVelocity = vel * fScaleToVelPerSec;
//      }
//
//      uiPrevMdIdx = uiMotionDataIdx;
//    }
//  }
//}
//
// WUInt32 WMotionMatchingComponent::FindBestKeyframe(
//  const TargetKeyframe& current, WVec3 vLeftFootPosition, WVec3 vRightFootPosition, WVec3 vTargetDir) const
//{
//  float fClosest = 1000000000.0f;
//  WUInt32 uiClosest = 0xFFFFFFFFu;
//
//  const float fDirWeight = 3.0f;
//
//  for (WUInt32 i = 0; i < m_MotionData.GetCount(); ++i)
//  {
//    const auto& md = m_MotionData[i];
//
//    float penaltyMul = 1.1f;
//    float penaltyAdd = 100;
//
//    if (md.m_uiAnimClipIndex == current.m_uiAnimClip)
//    {
//      // do NOT allow to transition backwards to a keyframe within a certain range
//      if (md.m_uiKeyframeIndex < current.m_uiKeyframe && md.m_uiKeyframeIndex + 10 > current.m_uiKeyframe)
//        continue;
//
//      penaltyMul = 1.0f;
//
//      if (md.m_uiKeyframeIndex == current.m_uiKeyframe)
//      {
//        penaltyAdd = 0;
//        penaltyMul = 0.9f;
//      }
//    }
//
//    const float dirDist = WMath::Pow((md.m_vRootVelocity - vTargetDir).GetLength(), fDirWeight);
//    const float leftFootDist = (md.m_vLeftFootPosition - vLeftFootPosition).GetLengthSquared();
//    const float rightFootDist = (md.m_vRightFootPosition - vRightFootPosition).GetLengthSquared();
//
//    const float fScore = dirDist + (leftFootDist + rightFootDist) * penaltyMul + penaltyAdd;
//
//    if (fScore < fClosest)
//    {
//      fClosest = fScore;
//      uiClosest = i;
//    }
//  }
//
//  return uiClosest;
//}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_MotionMatchingComponent);

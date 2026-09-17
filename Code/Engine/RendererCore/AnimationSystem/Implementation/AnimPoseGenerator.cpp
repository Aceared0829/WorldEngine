#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/base/span.h>

void WAnimPoseGenerator::Reset(const WSkeletonResource* pSkeleton, WGameObject* pTarget)
{
  m_pSkeleton = pSkeleton;
  m_pTargetGameObject = pTarget;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;
  m_FinalCommand = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsRestPose.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsSampleEventTrack.Clear();
  m_CommandsAimIK.Clear();
  m_CommandsTwoBoneIK.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  // m_UsedModelTransforms.Clear();
  // m_SamplingCaches.Clear();
}

static W_ALWAYS_INLINE WAnimPoseGeneratorCommandID CreateCommandID(WAnimPoseGeneratorCommandType type, WUInt32 uiIndex)
{
  return (static_cast<WUInt32>(type) << 24u) | uiIndex;
}

static W_ALWAYS_INLINE WUInt32 GetCommandIndex(WAnimPoseGeneratorCommandID id)
{
  return static_cast<WUInt32>(id) & 0x00FFFFFFu;
}

static W_ALWAYS_INLINE WAnimPoseGeneratorCommandType GetCommandType(WAnimPoseGeneratorCommandID id)
{
  return static_cast<WAnimPoseGeneratorCommandType>(static_cast<WUInt32>(id) >> 24u);
}

WAnimPoseGeneratorCommandSampleTrack& WAnimPoseGenerator::AllocCommandSampleTrack(WUInt32 uiDeterministicID)
{
  auto& cmd = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::SampleTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleTrack.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;
  cmd.m_uiUniqueID = uiDeterministicID;

  return cmd;
}

WAnimPoseGeneratorCommandRestPose& WAnimPoseGenerator::AllocCommandRestPose()
{
  auto& cmd = m_CommandsRestPose.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::RestPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsRestPose.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

WAnimPoseGeneratorCommandCombinePoses& WAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::CombinePoses;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsCombinePoses.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

WAnimPoseGeneratorCommandLocalToModelPose& WAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::LocalToModelPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsLocalToModelPose.GetCount() - 1);
  cmd.m_ModelPoseOutput = m_ModelPoseCounter++;

  return cmd;
}

WAnimPoseGeneratorCommandSampleEventTrack& WAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

  return cmd;
}

WAnimPoseGeneratorCommandAimIK& WAnimPoseGenerator::AllocCommandAimIK()
{
  auto& cmd = m_CommandsAimIK.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::AimIK;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsAimIK.GetCount() - 1);

  return cmd;
}

WAnimPoseGeneratorCommandTwoBoneIK& WAnimPoseGenerator::AllocCommandTwoBoneIK()
{
  auto& cmd = m_CommandsTwoBoneIK.ExpandAndGetRef();
  cmd.m_Type = WAnimPoseGeneratorCommandType::TwoBoneIK;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsTwoBoneIK.GetCount() - 1);

  return cmd;
}

WAnimPoseGenerator::WAnimPoseGenerator() = default;

WAnimPoseGenerator::~WAnimPoseGenerator()
{
  for (WUInt32 i = 0; i < m_SamplingCaches.GetCount(); ++i)
  {
    W_DEFAULT_DELETE(m_SamplingCaches.GetValue(i));
  }
  m_SamplingCaches.Clear();
}

void WAnimPoseGenerator::Validate() const
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)

  for (auto& cmd : m_CommandsSampleTrack)
  {
    W_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    // W_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    W_ASSERT_DEV(cmd.m_LocalPoseOutput != WInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    // W_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    W_ASSERT_DEV(cmd.m_LocalPoseOutput != WInvalidIndex, "Output pose not allocated.");
    W_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      W_ASSERT_DEV(type == WAnimPoseGeneratorCommandType::SampleTrack || type == WAnimPoseGeneratorCommandType::CombinePoses || type == WAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    W_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    W_ASSERT_DEV(cmd.m_ModelPoseOutput != WInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      W_ASSERT_DEV(type == WAnimPoseGeneratorCommandType::SampleTrack || type == WAnimPoseGeneratorCommandType::CombinePoses || type == WAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsAimIK)
  {
    W_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      W_ASSERT_DEV(type == WAnimPoseGeneratorCommandType::LocalToModelPose || type == WAnimPoseGeneratorCommandType::AimIK || type == WAnimPoseGeneratorCommandType::TwoBoneIK, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsTwoBoneIK)
  {
    W_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      W_ASSERT_DEV(type == WAnimPoseGeneratorCommandType::LocalToModelPose || type == WAnimPoseGeneratorCommandType::AimIK || type == WAnimPoseGeneratorCommandType::TwoBoneIK, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    W_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  }

#endif
}

const WAnimPoseGeneratorCommand& WAnimPoseGenerator::GetCommand(WAnimPoseGeneratorCommandID id) const
{
  return const_cast<WAnimPoseGenerator*>(this)->GetCommand(id);
}

WAnimPoseGeneratorCommand& WAnimPoseGenerator::GetCommand(WAnimPoseGeneratorCommandID id)
{
  W_ASSERT_DEV(id != WInvalidIndex, "Invalid command ID");

  switch (GetCommandType(id))
  {
    case WAnimPoseGeneratorCommandType::SampleTrack:
      return m_CommandsSampleTrack[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::RestPose:
      return m_CommandsRestPose[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::AimIK:
      return m_CommandsAimIK[GetCommandIndex(id)];

    case WAnimPoseGeneratorCommandType::TwoBoneIK:
      return m_CommandsTwoBoneIK[GetCommandIndex(id)];

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  W_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

void WAnimPoseGenerator::UpdatePose(bool bRequestExternalPoseGeneration)
{
  if (m_FinalCommand == 0)
    return;

  W_PROFILE_SCOPE("WAnimPoseGenerator::UpdatePose");
  Validate();

  Execute(GetCommand(m_FinalCommand));

  m_bSendResultMsg = true;

  if (bRequestExternalPoseGeneration && m_pTargetGameObject)
  {
    WMsgInjectPoseCommands poseGenMsg;
    poseGenMsg.m_pGenerator = this;

    while (true)
    {
      poseGenMsg.m_uiOrderNext = 0xFFFF;

      m_pTargetGameObject->SendMessageRecursive(poseGenMsg);

      if (poseGenMsg.m_uiOrderNext == 0xFFFF)
        break;

      poseGenMsg.m_uiOrderNow = poseGenMsg.m_uiOrderNext;
    }

    // if there is a very late-stage pose generator (usually a ragdoll)
    // don't forward the pose as a WMsgAnimationPoseUpdated
    m_bSendResultMsg = (poseGenMsg.m_uiOrderNow < 0xFF00);

    // update the pose once again afterwards
    Execute(GetCommand(m_FinalCommand));
  }
}

void WAnimPoseGenerator::Execute(WAnimPoseGeneratorCommand& cmd)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id));
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case WAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandSampleTrack&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::RestPose:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandRestPose&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandLocalToModelPose&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandSampleEventTrack&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::AimIK:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandAimIK&>(cmd));
      break;

    case WAnimPoseGeneratorCommandType::TwoBoneIK:
      ExecuteCmd(static_cast<WAnimPoseGeneratorCommandTwoBoneIK&>(cmd));
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandSampleTrack& cmd)
{
  WResourceLock<WAnimationClipResource> pResource(cmd.m_hAnimationClip, WResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  cmd.m_bAdditive = pResource->GetDescriptor().m_bAdditive;

  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  auto& pSampler = m_SamplingCaches[cmd.m_uiUniqueID];

  if (pSampler == nullptr)
  {
    pSampler = W_DEFAULT_NEW(ozz::animation::SamplingJob::Context);
  }

  if (pSampler->max_tracks() != ozzAnim.num_tracks())
  {
    pSampler->Resize(ozzAnim.num_tracks());
  }

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.context = pSampler;
  job.ratio = cmd.m_fNormalizedSamplePos;
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());

  if (!job.Validate())
    return;

  W_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandRestPose& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  const auto restPose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();

  transforms.CopyFrom(WArrayPtr<const ozz::math::SoaTransform>(restPose.begin(), (WUInt32)restPose.size()));
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  WTempHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;
  WTempHybridArray<ozz::animation::BlendingJob::Layer, 8> blAdd;

  for (WUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    if (cmdIn.GetType() == WAnimPoseGeneratorCommandType::SampleEventTrack)
      continue;

    ozz::animation::BlendingJob::Layer* layer = nullptr;

    switch (cmdIn.GetType())
    {
      case WAnimPoseGeneratorCommandType::SampleTrack:
      {
        if (static_cast<const WAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_bAdditive)
        {
          layer = &blAdd.ExpandAndGetRef();
        }
        else
        {
          layer = &bl.ExpandAndGetRef();
        }

        auto transform = AcquireLocalPoseTransforms(static_cast<const WAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case WAnimPoseGeneratorCommandType::RestPose:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform = AcquireLocalPoseTransforms(static_cast<const WAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case WAnimPoseGeneratorCommandType::CombinePoses:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform = AcquireLocalPoseTransforms(static_cast<const WAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    layer->weight = cmd.m_InputWeights[i];

    if (cmd.m_InputBoneWeights.GetCount() > i && !cmd.m_InputBoneWeights[i].IsEmpty())
    {
      layer->joint_weights = ozz::span(cmd.m_InputBoneWeights[i].GetPtr(), cmd.m_InputBoneWeights[i].GetEndPtr());
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold = 1.0f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.additive_layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(blAdd), end(blAdd));
  job.rest_pose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());
  W_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandLocalToModelPose& cmd)
{
  ozz::animation::LocalToModelJob job;

  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case WAnimPoseGeneratorCommandType::SampleTrack:
    {
      cmd.m_LocalPoseOutput = static_cast<const WAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput;
    }
    break;

    case WAnimPoseGeneratorCommandType::RestPose:
    {
      cmd.m_LocalPoseOutput = static_cast<const WAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput;
    }
    break;

    case WAnimPoseGeneratorCommandType::CombinePoses:
    {
      cmd.m_LocalPoseOutput = static_cast<const WAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput;
    }
    break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);
  job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());

  if (cmd.m_pSendLocalPoseMsgTo || m_pTargetGameObject)
  {
    WMsgAnimationPosePreparing msg;
    msg.m_pSkeleton = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = WMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (WUInt32)job.input.size());

    if (m_pTargetGameObject)
      m_pTargetGameObject->SendMessageRecursive(msg);
    else
      cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
  // This cast is safe because m_OutputPose points to m_UsedModelTransforms which is 16 byte aligned.
  W_ASSERT_DEBUG(WMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  W_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandSampleEventTrack& cmd)
{
  WResourceLock<WAnimationClipResource> pResource(cmd.m_hAnimationClip, WResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void MultiplySoATransformQuaternion(WUInt32 uiIndex, const ozz::math::SimdQuaternion& quat, WArrayPtr<ozz::math::SoaTransform>& ref_transforms)
{
  W_ASSERT_DEBUG(uiIndex < ref_transforms.GetCount() * 4, "Joint index out of bound.");

  // Convert SOA to AOS in order to perform quaternion multiplication, and get back to SOA.
  ozz::math::SoaTransform& soa_transform_ref = ref_transforms[uiIndex / 4];
  ozz::math::SimdQuaternion aos_quats[4];
  ozz::math::Transpose4x4(&soa_transform_ref.rotation.x, &aos_quats->xyzw);

  ozz::math::SimdQuaternion& aos_quat_ref = aos_quats[uiIndex & 3];
  aos_quat_ref = aos_quat_ref * quat;

  ozz::math::Transpose4x4(&aos_quats->xyzw, &soa_transform_ref.rotation.x);
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandAimIK& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case WAnimPoseGeneratorCommandType::LocalToModelPose:
    {
      const WAnimPoseGeneratorCommandLocalToModelPose& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case WAnimPoseGeneratorCommandType::AimIK:
    {
      const WAnimPoseGeneratorCommandAimIK& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandAimIK&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case WAnimPoseGeneratorCommandType::TwoBoneIK:
    {
      const WAnimPoseGeneratorCommandTwoBoneIK& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandTwoBoneIK&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  W_ASSERT_DEBUG(cmd.m_uiJointIdx < m_OutputPose.GetCount(), "Invalid joint index");

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  const WMat4* pJoint = &m_OutputPose[cmd.m_uiJointIdx];

  ozz::math::SimdQuaternion correction;
  bool bReached = false;

  const WVec3 vPoleOrigin = pJoint->GetTranslationVector();
  WVec3 vPoleVectorDir = (cmd.m_vPoleVectorPosition - vPoleOrigin).GetNormalized();

  if (cmd.m_bInversePoleVector)
  {
    vPoleVectorDir = -vPoleVectorDir;
  }

  // patch the local poses
  {
    ozz::animation::IKAimJob job;
    job.weight = cmd.m_fWeight;
    job.target = ozz::math::simd_float4::Load3PtrU(cmd.m_vTargetPosition.GetData());
    job.forward = ozz::math::simd_float4::Load3PtrU(cmd.m_vForwardVector.GetData());
    job.up = ozz::math::simd_float4::Load3PtrU(cmd.m_vUpVector.GetData());
    job.pole_vector = ozz::math::simd_float4::Load3PtrU(vPoleVectorDir.GetData());
    job.joint_correction = &correction;
    job.joint = reinterpret_cast<const ozz::math::Float4x4*>(pJoint);
    job.reached = &bReached;
    W_ASSERT_DEBUG(job.Validate(), "");
    job.Run();

    MultiplySoATransformQuaternion(cmd.m_uiJointIdx, correction, transform);
  }

  // rebuild the model poses
  {
    ozz::animation::LocalToModelJob job;
    job.from = (int)cmd.m_uiJointIdx;
    job.to = (int)cmd.m_uiRecalcModelPoseToJointIdx;
    job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    W_ASSERT_DEBUG(WMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
    job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    W_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  if (cmd.m_fDebugVisScale > 0.0f && m_pTargetGameObject)
  {
    const float fScale = cmd.m_fDebugVisScale;

    WMat4 mOff = WMat4::MakeIdentity();
    const WTransform tObj = m_pTargetGameObject->GetGlobalTransform();
    const WTransform& tRoot = m_pSkeleton->GetDescriptor().m_RootTransform;

    const WMat4 mToGlobal = tObj.GetAsMat4() * tRoot.GetAsMat4();
    const WTransform tToGlobal = tObj * tRoot;
    const WTransform tPoleArrow = WTransform::MakeGlobalTransform(tToGlobal, WTransform(vPoleOrigin));

    // Target position
    {
      WDebugRenderer::DrawLineSphere(m_pTargetGameObject->GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(cmd.m_vTargetPosition, fScale * 0.05f), bReached ? WColorScheme::LightUI(WColorScheme::Lime) : WColorScheme::LightUI(WColorScheme::Orange), tToGlobal);
    }

    // pole vector
    {
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.5f, WColorScheme::LightUI(WColorScheme::Cyan), tPoleArrow, vPoleVectorDir);
      WDebugRenderer::DrawCross(m_pTargetGameObject->GetWorld(), cmd.m_vPoleVectorPosition, fScale * 0.15f, WColorScheme::LightUI(WColorScheme::Cyan), tToGlobal);
    }

    const WMat4 mJointAxis = mToGlobal * *pJoint;

    // Joint coordinate system
    {
      mOff.SetTranslationVector(WVec3(-0.15f * fScale, 0, 0));
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Red), WTransform((mJointAxis * mOff).GetTranslationVector()), mJointAxis.TransformDirection(WVec3::MakeAxisX()));

      mOff.SetTranslationVector(WVec3(0, -0.15f * fScale, 0));
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Green), WTransform((mJointAxis * mOff).GetTranslationVector()), mJointAxis.TransformDirection(WVec3::MakeAxisY()));

      mOff.SetTranslationVector(WVec3(0, 0, -0.15f * fScale));
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Blue), WTransform((mJointAxis * mOff).GetTranslationVector()), mJointAxis.TransformDirection(WVec3::MakeAxisZ()));
    }

    // Joint Up axis
    {
      mOff.SetTranslationVector(-cmd.m_vUpVector * 0.17f * fScale);
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.34f, WColorScheme::LightUI(WColorScheme::Yellow), WTransform((mJointAxis * mOff).GetTranslationVector()), mJointAxis.TransformDirection(cmd.m_vUpVector));
    }

    // Joint Forward axis
    {
      WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Lime), WTransform(mJointAxis.GetTranslationVector()), mJointAxis.TransformDirection(cmd.m_vForwardVector));
    }
  }
}

void WAnimPoseGenerator::ExecuteCmd(WAnimPoseGeneratorCommandTwoBoneIK& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case WAnimPoseGeneratorCommandType::LocalToModelPose:
    {
      const WAnimPoseGeneratorCommandLocalToModelPose& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case WAnimPoseGeneratorCommandType::AimIK:
    {
      const WAnimPoseGeneratorCommandAimIK& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandAimIK&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case WAnimPoseGeneratorCommandType::TwoBoneIK:
    {
      const WAnimPoseGeneratorCommandTwoBoneIK& cmdIn2 = static_cast<const WAnimPoseGeneratorCommandTwoBoneIK&>(cmdIn);
      cmd.m_LocalPoseOutput = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput = cmdIn2.m_ModelPoseOutput;
      m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  W_ASSERT_DEBUG(cmd.m_uiJointIdxStart < m_OutputPose.GetCount(), "Invalid joint index");
  W_ASSERT_DEBUG(cmd.m_uiJointIdxMiddle < m_OutputPose.GetCount(), "Invalid joint index");
  W_ASSERT_DEBUG(cmd.m_uiJointIdxEnd < m_OutputPose.GetCount(), "Invalid joint index");

  const WMat4* pJointStart = &m_OutputPose[cmd.m_uiJointIdxStart];
  const WMat4* pJointMiddle = &m_OutputPose[cmd.m_uiJointIdxMiddle];
  const WMat4* pJointEnd = &m_OutputPose[cmd.m_uiJointIdxEnd];

  ozz::math::SimdQuaternion correctionStart, correctionMiddle;
  bool bReached = false;

  const WVec3 vPoleOrigin = WMath::Lerp(pJointStart->GetTranslationVector(), cmd.m_vTargetPosition, 0.5f);
  const WVec3 vPoleVectorDir = (cmd.m_vPoleVectorPosition - vPoleOrigin).GetNormalized();

  // patch the local poses
  {
    ozz::animation::IKTwoBoneJob job;
    job.reached = &bReached;
    job.weight = cmd.m_fWeight;
    job.target = ozz::math::simd_float4::Load3PtrU(cmd.m_vTargetPosition.GetData());
    job.start_joint = reinterpret_cast<const ozz::math::Float4x4*>(pJointStart);
    job.mid_joint = reinterpret_cast<const ozz::math::Float4x4*>(pJointMiddle);
    job.end_joint = reinterpret_cast<const ozz::math::Float4x4*>(pJointEnd);
    job.start_joint_correction = &correctionStart;
    job.mid_joint_correction = &correctionMiddle;
    job.mid_axis = ozz::math::simd_float4::Load3PtrU(cmd.m_vMidAxis.GetData());
    job.pole_vector = ozz::math::simd_float4::Load3PtrU(vPoleVectorDir.GetData());
    job.soften = cmd.m_fSoften;
    job.twist_angle = cmd.m_TwistAngle.GetRadian();
    W_ASSERT_DEBUG(job.Validate(), "");
    job.Run();

    MultiplySoATransformQuaternion(cmd.m_uiJointIdxStart, correctionStart, transform);
    MultiplySoATransformQuaternion(cmd.m_uiJointIdxMiddle, correctionMiddle, transform);
  }

  // rebuild the model poses
  {
    ozz::animation::LocalToModelJob job;
    job.from = (int)cmd.m_uiJointIdxStart;
    job.to = (int)cmd.m_uiRecalcModelPoseToJointIdx;
    job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    W_ASSERT_DEBUG(WMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
    job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    W_ASSERT_DEBUG(job.Validate(), "");
    job.Run();


    if (m_pTargetGameObject && cmd.m_fDebugVisScale > 0.0f)
    {
      const float fScale = cmd.m_fDebugVisScale;

      WMat4 mOff = WMat4::MakeIdentity();
      const WTransform tObj = m_pTargetGameObject->GetGlobalTransform();
      const WTransform& tRoot = m_pSkeleton->GetDescriptor().m_RootTransform;

      const WMat4 mToGlobal = tObj.GetAsMat4() * tRoot.GetAsMat4();
      const WTransform tToGlobal = tObj * tRoot;
      const WTransform tPoleArrow = WTransform::MakeGlobalTransform(tToGlobal, WTransform(vPoleOrigin));

      // Target position
      {
        WDebugRenderer::DrawLineSphere(m_pTargetGameObject->GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(cmd.m_vTargetPosition, fScale * 0.05f), bReached ? WColorScheme::LightUI(WColorScheme::Lime) : WColorScheme::LightUI(WColorScheme::Orange), tToGlobal);
      }

      // Start joint
      {
        WDebugRenderer::DrawLineSphere(m_pTargetGameObject->GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(pJointStart->GetTranslationVector(), fScale * 0.07f), WColorScheme::LightUI(WColorScheme::Grape), tToGlobal);
      }

      // pole vector
      {
        WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.5f, WColorScheme::LightUI(WColorScheme::Cyan), tPoleArrow, vPoleVectorDir);
        WDebugRenderer::DrawCross(m_pTargetGameObject->GetWorld(), cmd.m_vPoleVectorPosition, fScale * 0.15f, WColorScheme::LightUI(WColorScheme::Cyan), tToGlobal);
      }

      const WMat4 mMidAxis = mToGlobal * *pJointMiddle;

      // Mid coordinate system
      {
        mOff.SetTranslationVector(WVec3(-0.15f * fScale, 0, 0));
        WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Red), WTransform((mMidAxis * mOff).GetTranslationVector()), mMidAxis.TransformDirection(WVec3::MakeAxisX()));

        mOff.SetTranslationVector(WVec3(0, -0.15f * fScale, 0));
        WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Green), WTransform((mMidAxis * mOff).GetTranslationVector()), mMidAxis.TransformDirection(WVec3::MakeAxisY()));

        mOff.SetTranslationVector(WVec3(0, 0, -0.15f * fScale));
        WDebugRenderer::DrawArrow(m_pTargetGameObject->GetWorld(), fScale * 0.3f, WColorScheme::LightUI(WColorScheme::Blue), WTransform((mMidAxis * mOff).GetTranslationVector()), mMidAxis.TransformDirection(WVec3::MakeAxisZ()));
      }

      // Mid axis
      {
        WQuat qTilt = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), cmd.m_vMidAxis);
        mOff.SetTranslationVector(-cmd.m_vMidAxis * 0.15f * fScale);
        mOff.SetRotationalPart(qTilt.GetAsMat3());
        WDebugRenderer::DrawCylinder(m_pTargetGameObject->GetWorld(), fScale * 0.01f, fScale * 0.001f, fScale * 0.3f, WColor::MakeZero(), WColorScheme::LightUI(WColorScheme::Gray), mMidAxis * mOff);
      }
    }
  }
}

void WAnimPoseGenerator::SampleEventTrack(const WAnimationClipResource* pResource, WAnimPoseEventTrackSampleMode mode, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == WAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const WTime duration = pResource->GetDescriptor().GetDuration();

  const WTime tPrev = fPrevPos * duration;
  const WTime tNow = fCurPos * duration;
  const WTime tStart = WTime::MakeZero();
  const WTime tEnd = duration + WTime::MakeFromSeconds(1.0); // sampling position is EXCLUSIVE

  WTempHybridArray<WHashedString, 16> events;

  switch (mode)
  {
    case WAnimPoseEventTrackSampleMode::OnlyBetween:
      et.Sample(tPrev, tNow, events);
      break;

    case WAnimPoseEventTrackSampleMode::LoopAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tStart, tNow, events);
      break;

    case WAnimPoseEventTrackSampleMode::LoopAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

    case WAnimPoseEventTrackSampleMode::BounceAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tEnd, tNow, events);
      break;

    case WAnimPoseEventTrackSampleMode::BounceAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  WMsgGenericEvent msg;

  for (const auto& hs : events)
  {
    msg.m_sMessage = hs;

    m_pTargetGameObject->PostEventMessage(msg, nullptr, WTime::MakeZero());
  }
}

WArrayPtr<ozz::math::SoaTransform> WAnimPoseGenerator::AcquireLocalPoseTransforms(WAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id].IsEmpty())
  {
    using T = ozz::math::SoaTransform;
    const WUInt32 num = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints();
    m_UsedLocalTransforms[id] = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), T, num);
  }

  return m_UsedLocalTransforms[id];
}

WArrayPtr<WMat4> WAnimPoseGenerator::AcquireModelPoseTransforms(WAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  m_UsedModelTransforms[id].SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return m_UsedModelTransforms[id];
}

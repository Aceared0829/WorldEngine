#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <ozz/animation/runtime/skeleton.h>

WMutex WAnimController::s_SharedDataMutex;
WHashTable<WString, WSharedPtr<WAnimGraphSharedBoneWeights>> WAnimController::s_SharedBoneWeights;

WAnimController::WAnimController() = default;
WAnimController::~WAnimController() = default;

void WAnimController::Initialize(const WSkeletonResourceHandle& hSkeleton, WAnimPoseGenerator& ref_poseGenerator, const WSharedPtr<WBlackboard>& pBlackboard /*= nullptr*/)
{
  m_Instances.Clear();
  m_PinDataBoneWeights.Clear();
  m_PinDataLocalTransforms.Clear();
  m_PinDataModelTransforms.Clear();
  m_AnimationClipMapping.Clear();
  m_CurrentLocalTransformOutputs.Clear();
  m_pBlackboard.Clear();
  m_BlendMask.Clear();
  m_pPoseGenerator = nullptr;
  m_pCurrentModelTransforms = nullptr;
  m_hSkeleton = {};
  m_vRootMotion.SetZero();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;
}

void WAnimController::GetRootMotion(WVec3& ref_vTranslation, WAngle& ref_rotationX, WAngle& ref_rotationY, WAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

bool WAnimController::Update(WTime diff, WGameObject* pTarget, bool bEnableIK)
{
  if (!m_hSkeleton.IsValid())
    return false;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
    return false;

  m_pCurrentModelTransforms = nullptr;

  m_CurrentLocalTransformOutputs.Clear();

  m_vRootMotion = WVec3::MakeZero();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_FinalCurveValues.Clear();

  m_pPoseGenerator->Reset(pSkeleton.GetPointer(), pTarget);

  m_PinDataBoneWeights.Clear();
  m_PinDataLocalTransforms.Clear();
  m_PinDataModelTransforms.Clear();

  for (auto& inst : m_Instances)
  {
    inst.m_pInstance->Update(*this, diff, pTarget, pSkeleton.GetPointer());
  }

  GenerateLocalResultProcessors(pSkeleton.GetPointer());

  GetPoseGenerator().UpdatePose(bEnableIK);

  // send custom curve values to the game object
  for (const auto& fcv : m_FinalCurveValues)
  {
    if (fcv.m_fTotalWeight > 0.0f)
    {
      WMsgAnimationCurveValue msg;
      msg.m_sCurveName = fcv.m_sName;
      msg.m_fMin = fcv.m_fMin;
      msg.m_fMax = fcv.m_fMax;
      msg.m_fAverage = fcv.m_fWeightedSum / fcv.m_fTotalWeight;
      pTarget->PostEventMessage(msg, nullptr, WTime::MakeZero());
    }
  }

  if (GetPoseGenerator().ShouldSendPoseResultMsg())
  {
    if (auto newPose = GetPoseGenerator().GetCurrentPose(); !newPose.IsEmpty())
    {
      WMsgAnimationPoseUpdated msg;
      msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
      msg.m_ModelTransforms = newPose;

      // TODO: root transform has to be applied first, only then can the world-space IK be done, and then the pose can be finalized
      msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;

      // recursive, so that objects below the mesh can also listen in on these changes
      // for example bone attachments
      pTarget->SendMessageRecursive(msg);

      return msg.m_bContinueAnimating;
    }
  }

  return true;
}

void WAnimController::SetOutputModelTransform(WAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void WAnimController::SetRootMotion(const WVec3& vTranslation, WAngle rotationX, WAngle rotationY, WAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

void WAnimController::AddOutputLocalTransforms(WAnimGraphPinDataLocalTransforms* pLocalTransforms)
{
  m_CurrentLocalTransformOutputs.PushBack(pLocalTransforms->m_uiOwnIndex);
}

WSharedPtr<WAnimGraphSharedBoneWeights> WAnimController::CreateBoneWeights(const char* szUniqueName, const WSkeletonResource& skeleton, WDelegate<void(WAnimGraphSharedBoneWeights&)> fill)
{
  W_LOCK(s_SharedDataMutex);

  WSharedPtr<WAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = W_DEFAULT_NEW(WAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    WMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}

void WAnimController::GenerateLocalResultProcessors(const WSkeletonResource* pSkeleton)
{
  if (m_CurrentLocalTransformOutputs.IsEmpty())
    return;

  WAnimGraphPinDataLocalTransforms* pOut = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[0]];

  // combine multiple outputs
  if (m_CurrentLocalTransformOutputs.GetCount() > 1 || pOut->m_pWeights != nullptr)
  {
    const WUInt32 m_uiMaxPoses = 6; // TODO

    pOut = AddPinDataLocalTransforms();
    pOut->m_vRootMotion.SetZero();

    float fSummedRootMotionWeight = 0.0f;

    // TODO: skip blending, if only a single animation is played
    // unless the weight is below 1.0 and the bind pose should be faded in

    auto& cmd = GetPoseGenerator().AllocCommandCombinePoses();

    struct PinWeight
    {
      WUInt32 m_uiPinIdx;
      float m_fPinWeight = 0.0f;
    };

    WTempHybridArray<PinWeight, 16> pw;
    pw.SetCount(m_CurrentLocalTransformOutputs.GetCount());

    for (WUInt32 i = 0; i < m_CurrentLocalTransformOutputs.GetCount(); ++i)
    {
      pw[i].m_uiPinIdx = i;

      const WAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[i]];

      if (pTransforms != nullptr)
      {
        pw[i].m_fPinWeight = pTransforms->m_fOverallWeight;

        if (pTransforms->m_pWeights)
        {
          pw[i].m_fPinWeight *= pTransforms->m_pWeights->m_fOverallWeight;
        }
      }
    }

    if (pw.GetCount() > m_uiMaxPoses)
    {
      pw.Sort([](const PinWeight& lhs, const PinWeight& rhs)
        { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
      pw.SetCount(m_uiMaxPoses);
    }

    WArrayPtr<const ozz::math::SimdFloat4> invWeights;

    for (const auto& in : pw)
    {
      const WAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

      if (in.m_fPinWeight > 0 && pTransforms->m_pWeights)
      {
        // only initialize and use the inverse mask, when it is actually needed
        if (invWeights.IsEmpty())
        {
          m_BlendMask.SetCountUninitialized(pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

          for (auto& sj : m_BlendMask)
          {
            sj = ozz::math::simd_float4::one();
          }

          invWeights = m_BlendMask;
        }

        const ozz::math::SimdFloat4 factor = ozz::math::simd_float4::Load1(in.m_fPinWeight);

        const WArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

        for (WUInt32 i = 0; i < m_BlendMask.GetCount(); ++i)
        {
          const auto& weight = weights[i];
          auto& mask = m_BlendMask[i];

          const auto oneMinusWeight = ozz::math::NMAdd(factor, weight, ozz::math::simd_float4::one());

          mask = ozz::math::Min(mask, oneMinusWeight);
        }
      }
    }

    for (const auto& in : pw)
    {
      if (in.m_fPinWeight > 0)
      {
        const WAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

        if (pTransforms->m_pWeights)
        {
          const WArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

          cmd.m_InputBoneWeights.PushBack(weights);
        }
        else
        {
          cmd.m_InputBoneWeights.PushBack(invWeights);
        }

        if (pTransforms->m_bUseRootMotion)
        {
          fSummedRootMotionWeight += in.m_fPinWeight;
          pOut->m_vRootMotion += pTransforms->m_vRootMotion * in.m_fPinWeight;

          // TODO: combining quaternions is mathematically tricky
          // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

          pOut->m_bUseRootMotion = true;
        }

        // accumulate custom curve values weighted by pin weight
        for (const auto& cc : pTransforms->m_CustomCurveValues)
        {
          FinalCurveValue* pFinal = nullptr;
          for (auto& fcv : m_FinalCurveValues)
          {
            if (fcv.m_sName == cc.m_sName)
            {
              pFinal = &fcv;
              break;
            }
          }
          if (pFinal == nullptr)
          {
            pFinal = &m_FinalCurveValues.ExpandAndGetRef();
            pFinal->m_sName = cc.m_sName;
            pFinal->m_fMin = cc.m_fValue;
            pFinal->m_fMax = cc.m_fValue;
          }

          pFinal->m_fWeightedSum += cc.m_fValue * in.m_fPinWeight;
          pFinal->m_fTotalWeight += in.m_fPinWeight;
          pFinal->m_fMin = WMath::Min(pFinal->m_fMin, cc.m_fValue);
          pFinal->m_fMax = WMath::Max(pFinal->m_fMax, cc.m_fValue);
        }

        cmd.m_Inputs.PushBack(pTransforms->m_CommandID);
        cmd.m_InputWeights.PushBack(in.m_fPinWeight);
      }
    }

    if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
    {
      pOut->m_vRootMotion /= fSummedRootMotionWeight;
    }

    pOut->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    // Single output with no bone weight mask: collect curve values directly from the one output.
    for (const auto& cc : pOut->m_CustomCurveValues)
    {
      FinalCurveValue* pFinal = nullptr;
      for (auto& fcv : m_FinalCurveValues)
      {
        if (fcv.m_sName == cc.m_sName)
        {
          pFinal = &fcv;
          break;
        }
      }
      if (pFinal == nullptr)
      {
        pFinal = &m_FinalCurveValues.ExpandAndGetRef();
        pFinal->m_sName = cc.m_sName;
        pFinal->m_fMin = cc.m_fValue;
        pFinal->m_fMax = cc.m_fValue;
      }
      pFinal->m_fWeightedSum += cc.m_fValue * pOut->m_fOverallWeight;
      pFinal->m_fTotalWeight += pOut->m_fOverallWeight;
      pFinal->m_fMin = WMath::Min(pFinal->m_fMin, cc.m_fValue);
      pFinal->m_fMax = WMath::Max(pFinal->m_fMax, cc.m_fValue);
    }
  }

  WAnimGraphPinDataModelTransforms* pModelTransform = AddPinDataModelTransforms();

  // local space to model space
  {
    if (pOut->m_bUseRootMotion)
    {
      pModelTransform->m_bUseRootMotion = true;
      pModelTransform->m_vRootMotion = pOut->m_vRootMotion;
    }

    auto& cmd = GetPoseGenerator().AllocCommandLocalToModelPose();
    cmd.m_Inputs.PushBack(pOut->m_CommandID);

    pModelTransform->m_CommandID = cmd.GetCommandID();
  }

  // model space to output
  {
    WVec3 rootMotion = WVec3::MakeZero();
    WAngle rootRotationX;
    WAngle rootRotationY;
    WAngle rootRotationZ;
    GetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);

    GetPoseGenerator().SetFinalCommand(pModelTransform->m_CommandID);

    if (pModelTransform->m_bUseRootMotion)
    {
      rootMotion += pModelTransform->m_vRootMotion;
      rootRotationX += pModelTransform->m_RootRotationX;
      rootRotationY += pModelTransform->m_RootRotationY;
      rootRotationZ += pModelTransform->m_RootRotationZ;
    }

    SetOutputModelTransform(pModelTransform);

    SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
  }
}

WAnimGraphPinDataBoneWeights* WAnimController::AddPinDataBoneWeights()
{
  WAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<WUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

WAnimGraphPinDataLocalTransforms* WAnimController::AddPinDataLocalTransforms()
{
  WAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<WUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

WAnimGraphPinDataModelTransforms* WAnimController::AddPinDataModelTransforms()
{
  WAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<WUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void WAnimController::AddAnimGraph(const WAnimGraphResourceHandle& hGraph)
{
  if (!hGraph.IsValid())
    return;

  for (auto& inst : m_Instances)
  {
    if (inst.m_hAnimGraph == hGraph)
      return;
  }

  WResourceLock<WAnimGraphResource> pAnimGraph(hGraph, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimGraph.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  auto& inst = m_Instances.ExpandAndGetRef();
  inst.m_hAnimGraph = hGraph;
  inst.m_pInstance = W_DEFAULT_NEW(WAnimGraphInstance);
  inst.m_pInstance->Configure(pAnimGraph->GetAnimationGraph());

  for (auto& clip : pAnimGraph->GetAnimationClipMapping())
  {
    bool bExisted = false;
    auto& info = m_AnimationClipMapping.FindOrAdd(clip.m_sClipName, &bExisted);
    if (!bExisted)
    {
      info.m_hClip = clip.m_hClip;
    }
  }

  for (auto& ig : pAnimGraph->GetIncludeGraphs())
  {
    AddAnimGraph(WResourceManager::LoadResource<WAnimGraphResource>(ig));
  }
}

const WAnimController::AnimClipInfo& WAnimController::GetAnimationClipInfo(WTempHashedString sClipName) const
{
  auto it = m_AnimationClipMapping.Find(sClipName);
  if (!it.IsValid())
    return m_InvalidClipInfo;

  return it.Value();
}

void WAnimController::SetAnimationClipInfo(const WHashedString& sClipName, const AnimClipInfo& info)
{
  m_AnimationClipMapping[sClipName] = info;
}

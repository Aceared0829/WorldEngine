#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Blending/SwitchPoseAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSwitchPoseAnimNode, 1, WRTTIDefaultAllocator<WSwitchPoseAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("TransitionDuration", m_TransitionDuration)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromMilliseconds(200))),
    W_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new WNoTemporaryTransactionsAttribute(), new WDynamicPinAttribute(), new WDefaultValueAttribute(2)),
    W_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new WHiddenAttribute(), new WDynamicPinAttribute("PosesCount")),
    W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Pose Blending"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Yellow)),
    new WTitleAttribute("Pose Switch"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WSwitchPoseAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_TransitionDuration;
  stream << m_uiPosesCount;

  W_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return W_SUCCESS;
}

WResult WSwitchPoseAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  W_IGNORE_UNUSED(version);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_TransitionDuration;
  stream >> m_uiPosesCount;

  W_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return W_SUCCESS;
}

void WSwitchPoseAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InIndex.IsConnected())
    return;

  WTempHybridArray<const WAnimGraphLocalPoseInputPin*, 12> pPins;
  for (WUInt32 i = 0; i < m_InPoses.GetCount(); ++i)
  {
    pPins.PushBack(&m_InPoses[i]);
  }

  // duplicate pin connections to fill up holes
  for (WUInt32 i = 1; i < pPins.GetCount(); ++i)
  {
    if (!pPins[i]->IsConnected())
      pPins[i] = pPins[i - 1];
  }
  for (WUInt32 i = pPins.GetCount(); i > 1; --i)
  {
    if (!pPins[i - 2]->IsConnected())
      pPins[i - 2] = pPins[i - 1];
  }

  if (pPins.IsEmpty() || !pPins[0]->IsConnected())
  {
    // this can only be the case if no pin is connected, at all
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const WInt8 iDstIdx = WMath::Clamp<WInt8>((WInt8)m_InIndex.GetNumber(ref_graph, 0), 0, pPins.GetCount() - 1);

  if (pInstance->m_iTransitionToIndex < 0)
  {
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_iTransitionFromIndex = iDstIdx;
  }

  pInstance->m_TransitionTime += tDiff;

  if (iDstIdx != pInstance->m_iTransitionToIndex)
  {
    if (iDstIdx == pInstance->m_iTransitionFromIndex)
    {
      // if we transition back to the previous index, just reverse the transition
      pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
      pInstance->m_iTransitionToIndex = iDstIdx;
      pInstance->m_TransitionTime = WMath::Max(WTime::MakeZero(), m_TransitionDuration - pInstance->m_TransitionTime);
    }
    else if (pInstance->m_TransitionTime < m_TransitionDuration * 0.5)
    {
      // if we are still in the first half of the transition, switch the target index,
      // but keep the source index and transition time
      pInstance->m_iTransitionToIndex = iDstIdx;
    }
    else
    {
      // otherwise just start a new transition from the current target to the new target
      pInstance->m_TransitionTime = WTime::MakeZero();
      pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
      pInstance->m_iTransitionToIndex = iDstIdx;
    }
  }

  if (pInstance->m_TransitionTime >= m_TransitionDuration)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
  }

  W_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex < (WInt32)pPins.GetCount(), "Invalid pose index");

  WInt8 iTransitionFromIndex = pInstance->m_iTransitionFromIndex;
  WInt8 iTransitionToIndex = pInstance->m_iTransitionToIndex;

  if (pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph) == nullptr)
  {
    // if the 'from' pose already stopped, just jump to the 'to' pose
    iTransitionFromIndex = iTransitionToIndex;
  }

  if (iTransitionFromIndex == iTransitionToIndex)
  {
    const WAnimGraphLocalPoseInputPin* pPinToForward = pPins[iTransitionToIndex];

    if (pPinToForward->GetPose(ref_controller, ref_graph) == nullptr)
      return;

    // AddPinDataLocalTransforms must come before GetPose: adding to the array may reallocate it,
    // which would invalidate any pointer previously obtained from it.
    WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();
    WAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);
    pLocalTransforms->m_CommandID = pDataToForward->m_CommandID;
    pLocalTransforms->m_pWeights = pDataToForward->m_pWeights;
    pLocalTransforms->m_fOverallWeight = pDataToForward->m_fOverallWeight;
    pLocalTransforms->m_vRootMotion = pDataToForward->m_vRootMotion;
    pLocalTransforms->m_bUseRootMotion = pDataToForward->m_bUseRootMotion;

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
  else
  {
    auto pPose0 = pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[iTransitionToIndex]->GetPose(ref_controller, ref_graph);

    if (pPose0 == nullptr || pPose1 == nullptr)
      return;

    // Copy the fields we need before AddPinDataLocalTransforms, which may reallocate the array
    // and invalidate pPose0 and pPose1.
    const WAnimPoseGeneratorCommandID pose0CmdID = pPose0->m_CommandID;
    const WAnimPoseGeneratorCommandID pose1CmdID = pPose1->m_CommandID;
    const bool bPose0UseRootMotion = pPose0->m_bUseRootMotion;
    const bool bPose1UseRootMotion = pPose1->m_bUseRootMotion;
    const WVec3 vPose0RootMotion = pPose0->m_vRootMotion;
    const WVec3 vPose1RootMotion = pPose1->m_vRootMotion;

    WAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp0 = (float)WMath::Clamp(pInstance->m_TransitionTime.GetSeconds() / m_TransitionDuration.GetSeconds(), 0.0, 1.0);
    const float fLerp = static_cast<float>(WMath::GetCurveValue_EaseInOutCubic(fLerp0));

    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pose0CmdID;
    cmd.m_Inputs[1] = pose1CmdID;

    pPinData->m_CommandID = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = bPose0UseRootMotion || bPose1UseRootMotion;
    pPinData->m_vRootMotion = WMath::Lerp(vPose0RootMotion, vPose1RootMotion, fLerp);

    m_OutPose.SetPose(ref_graph, pPinData);
  }
}

bool WSwitchPoseAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Blending_SwitchPoseAnimNode);

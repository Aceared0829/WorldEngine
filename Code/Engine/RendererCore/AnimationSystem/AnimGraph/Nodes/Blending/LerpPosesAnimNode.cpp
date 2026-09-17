#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Blending/LerpPosesAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLerpPosesAnimNode, 1, WRTTIDefaultAllocator<WLerpPosesAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Lerp", m_fLerp)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 3.0f)),
    W_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new WNoTemporaryTransactionsAttribute(), new WDynamicPinAttribute(), new WDefaultValueAttribute(2)),
    W_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new WHiddenAttribute(), new WDynamicPinAttribute("PosesCount")),
    W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Pose Blending"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Violet)),
    new WTitleAttribute("Lerp Poses"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLerpPosesAnimNode::WLerpPosesAnimNode() = default;
WLerpPosesAnimNode::~WLerpPosesAnimNode() = default;

WResult WLerpPosesAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fLerp;
  stream << m_uiPosesCount;

  W_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return W_SUCCESS;
}

WResult WLerpPosesAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fLerp;
  stream >> m_uiPosesCount;

  W_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return W_SUCCESS;
}

void WLerpPosesAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
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

  const float fIndex = WMath::Clamp((float)m_InLerp.GetNumber(ref_graph, m_fLerp), 0.0f, (float)pPins.GetCount() - 1.0f);

  if (WMath::Fraction(fIndex) == 0.0f)
  {
    const WAnimGraphLocalPoseInputPin* pPinToForward = pPins[(WInt32)WMath::Trunc(fIndex)];

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
    WAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp = WMath::Fraction(fIndex);

    auto pPose0 = pPins[(WInt32)WMath::Trunc(fIndex)]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[(WInt32)WMath::Trunc(fIndex) + 1]->GetPose(ref_controller, ref_graph);

    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pPose0->m_CommandID;
    cmd.m_Inputs[1] = pPose1->m_CommandID;

    pPinData->m_CommandID = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = pPose0->m_bUseRootMotion || pPose1->m_bUseRootMotion;
    pPinData->m_vRootMotion = WMath::Lerp(pPose0->m_vRootMotion, pPose1->m_vRootMotion, fLerp);

    m_OutPose.SetPose(ref_graph, pPinData);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Blending_LerpPosesAnimNode);

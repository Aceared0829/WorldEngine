#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Output/PoseResultAnimNode.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPoseResultAnimNode, 1, WRTTIDefaultAllocator<WPoseResultAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FadeDuration", m_FadeDuration)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromMilliseconds(200)), new WClampValueAttribute(WTime::MakeZero(), WTime::MakeFromSeconds(10))),
    W_MEMBER_PROPERTY("InPose", m_InPose)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("InTargetWeight", m_InTargetWeight)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("InFadeDuration", m_InFadeDuration)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutOnFadedOut", m_OutOnFadedOut)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutOnFadedIn", m_OutOnFadedIn)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutCurrentWeight", m_OutCurrentWeight)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Output"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Grape)),
    new WTitleAttribute("Pose Result"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPoseResultAnimNode::WPoseResultAnimNode() = default;
WPoseResultAnimNode::~WPoseResultAnimNode() = default;

WResult WPoseResultAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_FadeDuration;

  W_SUCCEED_OR_RETURN(m_InPose.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InTargetWeight.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InFadeDuration.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InWeights.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFadedOut.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFadedIn.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutCurrentWeight.Serialize(stream));

  return W_SUCCESS;
}

WResult WPoseResultAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_FadeDuration;

  W_SUCCEED_OR_RETURN(m_InPose.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InTargetWeight.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InFadeDuration.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InWeights.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFadedOut.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFadedIn.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutCurrentWeight.Deserialize(stream));

  return W_SUCCESS;
}

void WPoseResultAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_InPose.IsConnected())
    return;

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bWasInterpolating = pInstance->m_PlayTime < pInstance->m_EndTime;
  const float fNewTargetWeight = static_cast<float>(m_InTargetWeight.GetNumber(ref_graph, 1.0f));

  if (pInstance->m_fEndWeight != fNewTargetWeight)
  {
    // compute weight from previous frame
    if (bWasInterpolating)
    {
      const float f = (float)(pInstance->m_PlayTime.GetSeconds() / pInstance->m_EndTime.GetSeconds());
      pInstance->m_fStartWeight = WMath::Lerp(pInstance->m_fStartWeight, pInstance->m_fEndWeight, f);
    }
    else
    {
      pInstance->m_fStartWeight = pInstance->m_fEndWeight;
    }

    pInstance->m_fEndWeight = fNewTargetWeight;
    pInstance->m_PlayTime = WTime::MakeZero();
    pInstance->m_EndTime = WTime::MakeFromSeconds(m_InFadeDuration.GetNumber(ref_graph, m_FadeDuration.GetSeconds()));
  }

  float fCurrentWeight = 0.0f;
  pInstance->m_PlayTime += tDiff;

  if (pInstance->m_PlayTime >= pInstance->m_EndTime)
  {
    fCurrentWeight = pInstance->m_fEndWeight;

    if (bWasInterpolating && fCurrentWeight <= 0.0f)
    {
      m_OutOnFadedOut.SetTriggered(ref_graph);
    }
    if (bWasInterpolating && fCurrentWeight >= 1.0f)
    {
      m_OutOnFadedIn.SetTriggered(ref_graph);
    }
  }
  else
  {
    const float f = (float)(pInstance->m_PlayTime.GetSeconds() / pInstance->m_EndTime.GetSeconds());
    fCurrentWeight = WMath::Lerp(pInstance->m_fStartWeight, pInstance->m_fEndWeight, f);
  }

  m_OutCurrentWeight.SetNumber(ref_graph, fCurrentWeight);

  if (fCurrentWeight <= 0.0f)
    return;

  if (auto pCurrentLocalTransforms = m_InPose.GetPose(ref_controller, ref_graph))
  {
    if (pCurrentLocalTransforms->m_CommandID != WInvalidIndex)
    {
      WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      // Re-query: AddPinDataLocalTransforms may have reallocated the array, invalidating the pointer obtained above.
      pCurrentLocalTransforms = m_InPose.GetPose(ref_controller, ref_graph);

      pLocalTransforms->m_CommandID = pCurrentLocalTransforms->m_CommandID;
      pLocalTransforms->m_pWeights = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = pCurrentLocalTransforms->m_fOverallWeight * fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = pCurrentLocalTransforms->m_bUseRootMotion;
      pLocalTransforms->m_vRootMotion = pCurrentLocalTransforms->m_vRootMotion;
      pLocalTransforms->m_CustomCurveValues = pCurrentLocalTransforms->m_CustomCurveValues;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
  else
  {
    // if we are active, but the incoming pose isn't valid (anymore), use a rest pose as placeholder
    // this assumes that many animations return to the rest pose and if they are played up to the very end before fading out
    // they can be faded out by using the rest pose

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID = cmd.GetCommandID();
      pLocalTransforms->m_pWeights = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = false;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
}

bool WPoseResultAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Output_PoseResultAnimNode);

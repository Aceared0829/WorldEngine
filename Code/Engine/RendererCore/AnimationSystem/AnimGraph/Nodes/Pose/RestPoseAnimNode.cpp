#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Pose/RestPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRestPoseAnimNode, 1, WRTTIDefaultAllocator<WRestPoseAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("Rest Pose"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WRestPoseAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return W_SUCCESS;
}

WResult WRestPoseAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return W_SUCCESS;
}

void WRestPoseAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

  {
    WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = false;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Pose_RestPoseAnimNode);

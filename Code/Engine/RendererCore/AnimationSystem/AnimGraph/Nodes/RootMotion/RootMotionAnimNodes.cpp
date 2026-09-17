#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/RootMotion/RootMotionAnimNodes.h>

// clang-format off
 W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRootRotationAnimNode, 1, WRTTIDefaultAllocator<WRootRotationAnimNode>)
{
   W_BEGIN_PROPERTIES
   {
     W_MEMBER_PROPERTY("InRotateX", m_InRotateX)->AddAttributes(new WHiddenAttribute),
     W_MEMBER_PROPERTY("InRotateY", m_InRotateY)->AddAttributes(new WHiddenAttribute),
     W_MEMBER_PROPERTY("InRotateZ", m_InRotateZ)->AddAttributes(new WHiddenAttribute),
   }
   W_END_PROPERTIES;
   W_BEGIN_ATTRIBUTES
   {
     new WCategoryAttribute("Output"),
     new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Grape)),
     new WTitleAttribute("Root Rotation"),
   }
   W_END_ATTRIBUTES;
 }
 W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRootRotationAnimNode::WRootRotationAnimNode() = default;
WRootRotationAnimNode::~WRootRotationAnimNode() = default;

WResult WRootRotationAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InRotateX.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InRotateY.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InRotateZ.Serialize(stream));

  return W_SUCCESS;
}

WResult WRootRotationAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InRotateX.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InRotateY.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InRotateZ.Deserialize(stream));

  return W_SUCCESS;
}

void WRootRotationAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  WVec3 vRootMotion = WVec3::MakeZero();
  WAngle rootRotationX;
  WAngle rootRotationY;
  WAngle rootRotationZ;

  ref_controller.GetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);

  if (m_InRotateX.IsConnected())
  {
    rootRotationX += WAngle::MakeFromDegree(static_cast<float>(m_InRotateX.GetNumber(ref_graph)));
  }
  if (m_InRotateY.IsConnected())
  {
    rootRotationY += WAngle::MakeFromDegree(static_cast<float>(m_InRotateY.GetNumber(ref_graph)));
  }
  if (m_InRotateZ.IsConnected())
  {
    rootRotationZ += WAngle::MakeFromDegree(static_cast<float>(m_InRotateZ.GetNumber(ref_graph)));
  }

  ref_controller.SetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_RootMotion_RootMotionAnimNodes);

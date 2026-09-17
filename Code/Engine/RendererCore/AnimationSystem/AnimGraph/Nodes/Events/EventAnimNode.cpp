#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Events/EventAnimNode.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSendEventAnimNode, 1, WRTTIDefaultAllocator<WSendEventAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    W_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Events"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Orange)),
    new WTitleAttribute("Send Event: '{EventName}'"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WSendEventAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  W_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));

  return W_SUCCESS;
}

WResult WSendEventAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  W_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));

  return W_SUCCESS;
}

void WSendEventAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (m_sEventName.IsEmpty())
    return;

  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  WMsgGenericEvent msg;
  msg.m_sMessage = m_sEventName;

  pTarget->SendEventMessage(msg, nullptr);
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Events_EventAnimNode);

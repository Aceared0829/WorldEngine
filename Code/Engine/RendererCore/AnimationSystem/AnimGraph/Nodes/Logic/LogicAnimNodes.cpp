#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Logic/LogicAnimNodes.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogicAndAnimNode, 1, WRTTIDefaultAllocator<WLogicAndAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new WNoTemporaryTransactionsAttribute(), new WDynamicPinAttribute(), new WDefaultValueAttribute(2)),
    W_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new WHiddenAttribute(), new WDynamicPinAttribute("BoolCount")),
    W_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("AND"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLogicAndAnimNode::WLogicAndAnimNode() = default;
WLogicAndAnimNode::~WLogicAndAnimNode() = default;

WResult WLogicAndAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return W_SUCCESS;
}

WResult WLogicAndAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return W_SUCCESS;
}

void WLogicAndAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  bool res = true;

  for (const auto& pin : m_InBool)
  {
    if (!pin.GetBool(ref_graph, true))
    {
      res = false;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogicEventAndAnimNode, 1, WRTTIDefaultAllocator<WLogicEventAndAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutOnActivated", m_OutOnActivated)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("Event AND"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLogicEventAndAnimNode::WLogicEventAndAnimNode() = default;
WLogicEventAndAnimNode::~WLogicEventAndAnimNode() = default;

WResult WLogicEventAndAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnActivated.Serialize(stream));

  return W_SUCCESS;
}

WResult WLogicEventAndAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnActivated.Deserialize(stream));

  return W_SUCCESS;
}

void WLogicEventAndAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (m_InActivate.IsTriggered(ref_graph) && m_InBool.GetBool(ref_graph))
  {
    m_OutOnActivated.SetTriggered(ref_graph);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogicOrAnimNode, 1, WRTTIDefaultAllocator<WLogicOrAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new WNoTemporaryTransactionsAttribute(), new WDynamicPinAttribute(), new WDefaultValueAttribute(2)),
    W_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new WHiddenAttribute(), new WDynamicPinAttribute("BoolCount")),
    W_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("OR"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLogicOrAnimNode::WLogicOrAnimNode() = default;
WLogicOrAnimNode::~WLogicOrAnimNode() = default;

WResult WLogicOrAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return W_SUCCESS;
}

WResult WLogicOrAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return W_SUCCESS;
}

void WLogicOrAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  bool res = false;

  for (const auto& pin : m_InBool)
  {
    if (pin.GetBool(ref_graph, false))
    {
      res = true;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogicNotAnimNode, 1, WRTTIDefaultAllocator<WLogicNotAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("NOT"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLogicNotAnimNode::WLogicNotAnimNode() = default;
WLogicNotAnimNode::~WLogicNotAnimNode() = default;

WResult WLogicNotAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return W_SUCCESS;
}

WResult WLogicNotAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return W_SUCCESS;
}

void WLogicNotAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  const bool value = !m_InBool.GetBool(ref_graph);

  m_OutBool.SetBool(ref_graph, !value);
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Logic_LogicAnimNodes);

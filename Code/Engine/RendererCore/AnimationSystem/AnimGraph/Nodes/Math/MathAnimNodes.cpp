#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Math/MathAnimNodes.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMathExpressionAnimNode, 1, WRTTIDefaultAllocator<WMathExpressionAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression)->AddAttributes(new WDefaultValueAttribute("a*a + (b-c) / abs(d)")),
    W_MEMBER_PROPERTY("a", m_ValueAPin)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("b", m_ValueBPin)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("c", m_ValueCPin)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("d", m_ValueDPin)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("Result", m_ResultPin)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Math"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
    new WTitleAttribute("= {Expression}"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMathExpressionAnimNode::WMathExpressionAnimNode() = default;
WMathExpressionAnimNode::~WMathExpressionAnimNode() = default;

void WMathExpressionAnimNode::SetExpression(WString sExpr)
{
  m_sExpression = sExpr;
}

WString WMathExpressionAnimNode::GetExpression() const
{
  return m_sExpression;
}

WResult WMathExpressionAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sExpression;
  W_SUCCEED_OR_RETURN(m_ValueAPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueBPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueCPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueDPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_ResultPin.Serialize(stream));

  return W_SUCCESS;
}

WResult WMathExpressionAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sExpression;
  W_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return W_SUCCESS;
}

static WHashedString s_sA = WMakeHashedString("a");
static WHashedString s_sB = WMakeHashedString("b");
static WHashedString s_sC = WMakeHashedString("c");
static WHashedString s_sD = WMakeHashedString("d");

void WMathExpressionAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_mExpression.GetExpressionString().IsEmpty())
  {
    pInstance->m_mExpression.Reset(m_sExpression);
  }

  if (!pInstance->m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(ref_graph, 0);
    return;
  }

  WMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(ref_graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(ref_graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(ref_graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(ref_graph))},
    };

  float result = pInstance->m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(ref_graph, result);
}

bool WMathExpressionAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCompareNumberAnimNode, 1, WRTTIDefaultAllocator<WCompareNumberAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    W_ENUM_MEMBER_PROPERTY("Comparison", WComparisonOperator, m_Comparison),

    W_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("InReference", m_InReference)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("Compare: Number {Comparison} {ReferenceValue}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WCompareNumberAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  W_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InReference.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return W_SUCCESS;
}

WResult WCompareNumberAnimNode::DeserializeNode(WStreamReader& stream)
{
  auto version = stream.ReadVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  W_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InReference.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  if (version >= 2)
  {
    W_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));
  }

  return W_SUCCESS;
}

void WCompareNumberAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  const bool bIsTrue = WComparisonOperator::Compare<double>(m_Comparison, m_InNumber.GetNumber(ref_graph), m_InReference.GetNumber(ref_graph, m_fReferenceValue));

  m_OutIsTrue.SetBool(ref_graph, bIsTrue);
  m_OutIsFalse.SetBool(ref_graph, !bIsTrue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoolToNumberAnimNode, 1, WRTTIDefaultAllocator<WBoolToNumberAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FalseValue", m_fFalseValue)->AddAttributes(new WDefaultValueAttribute(0.0)),
    W_MEMBER_PROPERTY("TrueValue", m_fTrueValue)->AddAttributes(new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("Bool To Number"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoolToNumberAnimNode::WBoolToNumberAnimNode() = default;
WBoolToNumberAnimNode::~WBoolToNumberAnimNode() = default;

WResult WBoolToNumberAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fFalseValue;
  stream << m_fTrueValue;

  W_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return W_SUCCESS;
}

WResult WBoolToNumberAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fFalseValue;
  stream >> m_fTrueValue;

  W_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return W_SUCCESS;
}

void WBoolToNumberAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  m_OutNumber.SetNumber(ref_graph, m_InValue.GetBool(ref_graph) ? m_fTrueValue : m_fFalseValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoolToTriggerAnimNode, 1, WRTTIDefaultAllocator<WBoolToTriggerAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
    new WTitleAttribute("Bool To Event"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoolToTriggerAnimNode::WBoolToTriggerAnimNode() = default;
WBoolToTriggerAnimNode::~WBoolToTriggerAnimNode() = default;

WResult WBoolToTriggerAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));

  return W_SUCCESS;
}

WResult WBoolToTriggerAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));

  return W_SUCCESS;
}

bool WBoolToTriggerAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void WBoolToTriggerAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bIsTrueNow = m_InValue.GetBool(ref_graph);
  const WInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bIsTrueNow)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Math_MathAnimNodes);

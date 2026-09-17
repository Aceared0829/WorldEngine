#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Blackboard/BlackboardAnimNodes.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSetBlackboardNumberAnimNode, 1, WRTTIDefaultAllocator<WSetBlackboardNumberAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_MEMBER_PROPERTY("Number", m_fNumber),

    W_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Set Number: '{BlackboardEntry}' to {Number}"),
    new WCategoryAttribute("Blackboard"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WSetBlackboardNumberAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fNumber;

  W_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));

  return W_SUCCESS;
}

WResult WSetBlackboardNumberAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fNumber;

  W_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));

  return W_SUCCESS;
}

void WSetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WSetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WSetBlackboardNumberAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (m_InActivate.IsConnected() && !m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InNumber.GetNumber(ref_graph, m_fNumber));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGetBlackboardNumberAnimNode, 1, WRTTIDefaultAllocator<WGetBlackboardNumberAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),

    W_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Blackboard"),
    new WTitleAttribute("Get Number: '{BlackboardEntry}'"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WGetBlackboardNumberAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return W_SUCCESS;
}

WResult WGetBlackboardNumberAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return W_SUCCESS;
}

void WGetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WGetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WGetBlackboardNumberAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  WVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    WLog::Warning("AnimController::GetBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  m_OutNumber.SetNumber(ref_graph, value.ConvertTo<double>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCompareBlackboardNumberAnimNode, 1, WRTTIDefaultAllocator<WCompareBlackboardNumberAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    W_ENUM_MEMBER_PROPERTY("Comparison", WComparisonOperator, m_Comparison),

    W_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Blackboard"),
    new WTitleAttribute("Check: '{BlackboardEntry}' {Comparison} {ReferenceValue}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WCompareBlackboardNumberAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(3);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  W_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return W_SUCCESS;
}

WResult WCompareBlackboardNumberAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(3);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  W_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  if (version >= 3)
  {
    W_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));
  }

  return W_SUCCESS;
}

void WCompareBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WCompareBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WCompareBlackboardNumberAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const WVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    WLog::Warning("AnimController::CompareBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const double fValue = value.ConvertTo<double>();
  const bool bIsTrueNow = WComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue);
  const WInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  m_OutIsTrue.SetBool(ref_graph, bIsTrueNow);
  m_OutIsFalse.SetBool(ref_graph, !bIsTrueNow);

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

bool WCompareBlackboardNumberAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCheckBlackboardBoolAnimNode, 1, WRTTIDefaultAllocator<WCheckBlackboardBoolAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),

    W_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Blackboard"),
    new WTitleAttribute("Check Bool: '{BlackboardEntry}'"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WCheckBlackboardBoolAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return W_SUCCESS;
}

WResult WCheckBlackboardBoolAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return W_SUCCESS;
}

void WCheckBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WCheckBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WCheckBlackboardBoolAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const WVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    WLog::Warning("AnimController::CheckBlackboardBool: '{}' doesn't exist or isn't a bool type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bValue = value.ConvertTo<bool>();
  const WInt8 iIsTrueNow = bValue ? 1 : 0;

  m_OutBool.SetBool(ref_graph, bValue);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bValue)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool WCheckBlackboardBoolAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSetBlackboardBoolAnimNode, 1, WRTTIDefaultAllocator<WSetBlackboardBoolAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_MEMBER_PROPERTY("Bool", m_bBool),

    W_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Set Bool: '{BlackboardEntry}' to {Bool}"),
    new WCategoryAttribute("Blackboard"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WSetBlackboardBoolAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_bBool;

  W_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));

  return W_SUCCESS;
}

WResult WSetBlackboardBoolAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_bBool;

  W_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));

  return W_SUCCESS;
}

void WSetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WSetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WSetBlackboardBoolAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InBool.GetBool(ref_graph, m_bBool));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGetBlackboardBoolAnimNode, 1, WRTTIDefaultAllocator<WGetBlackboardBoolAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),

    W_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Blackboard"),
    new WTitleAttribute("Get Bool: '{BlackboardEntry}'"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WGetBlackboardBoolAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return W_SUCCESS;
}

WResult WGetBlackboardBoolAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return W_SUCCESS;
}

void WGetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WGetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WGetBlackboardBoolAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  WVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    WLog::Warning("AnimController::GetBlackboardBool: '{}' doesn't exist or can't be converted to bool.", m_sBlackboardEntry);
    return;
  }

  m_OutBool.SetBool(ref_graph, value.ConvertTo<bool>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOnBlackboardValueChangedAnimNode, 1, WRTTIDefaultAllocator<WOnBlackboardValueChangedAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),

    W_MEMBER_PROPERTY("OutOnValueChanged", m_OutOnValueChanged)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Blackboard"),
    new WTitleAttribute("OnChanged: '{BlackboardEntry}'"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Lime)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WOnBlackboardValueChangedAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutOnValueChanged.Serialize(stream));

  return W_SUCCESS;
}

WResult WOnBlackboardValueChangedAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  W_SUCCEED_OR_RETURN(m_OutOnValueChanged.Deserialize(stream));

  return W_SUCCESS;
}

void WOnBlackboardValueChangedAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* WOnBlackboardValueChangedAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void WOnBlackboardValueChangedAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const WBlackboard::Entry* pEntry = pBlackboard->GetEntry(m_sBlackboardEntry);

  if (pEntry == nullptr)
  {
    WLog::Warning("AnimController::OnBlackboardValueChanged: '{}' doesn't exist.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_uiChangeCounter == pEntry->m_uiChangeCounter)
    return;

  if (pInstance->m_uiChangeCounter != WInvalidIndex)
  {
    m_OutOnValueChanged.SetTriggered(ref_graph);
  }

  pInstance->m_uiChangeCounter = pEntry->m_uiChangeCounter;
}

bool WOnBlackboardValueChangedAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Blackboard_BlackboardAnimNodes);

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphPin, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphInputPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphOutputPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WAnimGraphPin::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_iPinIndex;
  inout_stream << m_uiNumConnections;
  return W_SUCCESS;
}

WResult WAnimGraphPin::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_iPinIndex;
  inout_stream >> m_uiNumConnections;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphTriggerInputPin, 1, WRTTIDefaultAllocator<WAnimGraphTriggerInputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphTriggerOutputPin, 1, WRTTIDefaultAllocator<WAnimGraphTriggerOutputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WAnimGraphTriggerOutputPin::SetTriggered(WAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[WAnimGraphPin::Trigger][m_iPinIndex];


  const WInt8 offset = +1; // bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (WUInt16 idx : map)
  {
    ref_graph.m_pTriggerInputPinStates[idx] += offset;
  }
}

bool WAnimGraphTriggerInputPin::IsTriggered(WAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] > 0;
}

bool WAnimGraphTriggerInputPin::AreAllTriggered(WAnimGraphInstance& ref_graph) const
{
  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphNumberInputPin, 1, WRTTIDefaultAllocator<WAnimGraphNumberInputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphNumberOutputPin, 1, WRTTIDefaultAllocator<WAnimGraphNumberOutputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double WAnimGraphNumberInputPin::GetNumber(WAnimGraphInstance& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_pNumberInputPinStates[m_iPinIndex];
}

void WAnimGraphNumberOutputPin::SetNumber(WAnimGraphInstance& ref_graph, double value) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[WAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (WUInt16 idx : map)
  {
    ref_graph.m_pNumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphBoolInputPin, 1, WRTTIDefaultAllocator<WAnimGraphBoolInputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphBoolOutputPin, 1, WRTTIDefaultAllocator<WAnimGraphBoolOutputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WAnimGraphBoolInputPin::GetBool(WAnimGraphInstance& ref_graph, bool bFallback /*= false */) const
{
  if (m_iPinIndex < 0)
    return bFallback;

  return ref_graph.m_pBoolInputPinStates[m_iPinIndex];
}

void WAnimGraphBoolOutputPin::SetBool(WAnimGraphInstance& ref_graph, bool bValue) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[WAnimGraphPin::Bool][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (WUInt16 idx : map)
  {
    ref_graph.m_pBoolInputPinStates[idx] = bValue;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphBoneWeightsInputPin, 1, WRTTIDefaultAllocator<WAnimGraphBoneWeightsInputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphBoneWeightsOutputPin, 1, WRTTIDefaultAllocator<WAnimGraphBoneWeightsOutputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimGraphPinDataBoneWeights* WAnimGraphBoneWeightsInputPin::GetWeights(WAnimController& ref_controller, WAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_controller.m_PinDataBoneWeights[ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex]];
}

void WAnimGraphBoneWeightsOutputPin::SetWeights(WAnimGraphInstance& ref_graph, WAnimGraphPinDataBoneWeights* pWeights) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[WAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (WUInt16 idx : map)
  {
    ref_graph.m_pBoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphLocalPoseInputPin, 1, WRTTIDefaultAllocator<WAnimGraphLocalPoseInputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphLocalPoseOutputPin, 1, WRTTIDefaultAllocator<WAnimGraphLocalPoseOutputPin>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimGraphPinDataLocalTransforms* WAnimGraphLocalPoseInputPin::GetPose(WAnimController& ref_controller, WAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_controller.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void WAnimGraphLocalPoseOutputPin::SetPose(WAnimGraphInstance& ref_graph, WAnimGraphPinDataLocalTransforms* pPose) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[WAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (WUInt16 idx : map)
  {
    ref_graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);

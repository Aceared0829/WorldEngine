#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

WAnimGraphInstance::WAnimGraphInstance() = default;

WAnimGraphInstance::~WAnimGraphInstance()
{
  if (m_pAnimGraph)
  {
    m_pAnimGraph->GetInstanceDataAlloator().DestructAndDeallocate(m_InstanceData);
  }
}

void WAnimGraphInstance::Configure(const WAnimGraph& animGraph)
{
  m_pAnimGraph = &animGraph;

  m_InstanceData = m_pAnimGraph->GetInstanceDataAlloator().AllocateAndConstruct();

  // EXTEND THIS if a new type is introduced
  m_pTriggerInputPinStates = (WInt8*)WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Trigger]);
  m_pNumberInputPinStates = (double*)WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Number]);
  m_pBoolInputPinStates = (bool*)WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Bool]);
  m_pBoneWeightInputPinStates = (WUInt16*)WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[WAnimGraphPin::Type::BoneWeights]);
  m_pModelPoseInputPinStates = (WUInt16*)WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[WAnimGraphPin::Type::ModelPose]);

  m_LocalPoseInputPinStates.SetCount(animGraph.m_uiInputPinCounts[WAnimGraphPin::Type::LocalPose]);
}

void WAnimGraphInstance::Update(WAnimController& ref_controller, WTime diff, WGameObject* pTarget, const WSkeletonResource* pSekeltonResource)
{
  // reset all pin states
  {
    // EXTEND THIS if a new type is introduced

    WMemoryUtils::ZeroFill(m_pTriggerInputPinStates, m_pAnimGraph->m_uiInputPinCounts[WAnimGraphPin::Type::Trigger]);
    WMemoryUtils::ZeroFill(m_pNumberInputPinStates, m_pAnimGraph->m_uiInputPinCounts[WAnimGraphPin::Type::Number]);
    WMemoryUtils::ZeroFill(m_pBoolInputPinStates, m_pAnimGraph->m_uiInputPinCounts[WAnimGraphPin::Type::Bool]);
    WMemoryUtils::ZeroFill(m_pBoneWeightInputPinStates, m_pAnimGraph->m_uiInputPinCounts[WAnimGraphPin::Type::BoneWeights]);
    WMemoryUtils::PatternFill(m_pModelPoseInputPinStates, 0xFF, m_pAnimGraph->m_uiInputPinCounts[WAnimGraphPin::Type::ModelPose]);

    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
  }

  for (const auto& pNode : m_pAnimGraph->GetNodes())
  {
    pNode->Step(ref_controller, *this, diff, pSekeltonResource, pTarget);
  }
}

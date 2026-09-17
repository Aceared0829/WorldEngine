#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/StateMachine/StateMachineBuiltins.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_NestedStateMachine, 1, WRTTIDefaultAllocator<WStateMachineState_NestedStateMachine>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_StateMachine", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    W_MEMBER_PROPERTY("KeepCurrentStateOnExit", m_bKeepCurrentStateOnExit),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState_NestedStateMachine::WStateMachineState_NestedStateMachine(WStringView sName)
  : WStateMachineState(sName)
{
}

WStateMachineState_NestedStateMachine::~WStateMachineState_NestedStateMachine() = default;

void WStateMachineState_NestedStateMachine::OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;

  if (pStateMachineInstance == nullptr)
  {
    if (m_hResource.IsValid() == false)
      return;

    WResourceLock<WStateMachineResource> pStateMachineResource(m_hResource, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pStateMachineResource.GetAcquireResult() != WResourceAcquireResult::Final)
    {
      WLog::Error("Failed to load state machine '{}'", GetResource().GetResourceID());
      return;
    }

    pStateMachineInstance = pStateMachineResource->CreateInstance(ref_instance.GetOwner());
    pStateMachineInstance->SetBlackboard(ref_instance.GetBlackboard());
  }

  if (pStateMachineInstance->GetCurrentState() == nullptr)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void WStateMachineState_NestedStateMachine::OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const
{
  if (m_bKeepCurrentStateOnExit == false)
  {
    auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
    if (pStateMachineInstance != nullptr)
    {
      pStateMachineInstance->SetState(nullptr).IgnoreResult();
    }
  }
}

void WStateMachineState_NestedStateMachine::Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update(deltaTime);
  }
}

WResult WStateMachineState_NestedStateMachine::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_hResource;
  inout_stream << m_sInitialState;
  inout_stream << m_bKeepCurrentStateOnExit;
  return W_SUCCESS;
}

WResult WStateMachineState_NestedStateMachine::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_hResource;
  inout_stream >> m_sInitialState;
  inout_stream >> m_bKeepCurrentStateOnExit;
  return W_SUCCESS;
}

bool WStateMachineState_NestedStateMachine::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void WStateMachineState_NestedStateMachine::SetResource(const WStateMachineResourceHandle& hResource)
{
  m_hResource = hResource;
}

void WStateMachineState_NestedStateMachine::SetInitialState(const char* szName)
{
  m_sInitialState.Assign(szName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_Compound, 1, WRTTIDefaultAllocator<WStateMachineState_Compound>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("SubStates", m_SubStates)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState_Compound::WStateMachineState_Compound(WStringView sName)
  : WStateMachineState(sName)
{
}

WStateMachineState_Compound::~WStateMachineState_Compound()
{
  for (auto pSubState : m_SubStates)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

void WStateMachineState_Compound::OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const
{
  auto pData = static_cast<WStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  for (WUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnEnter(ref_instance, pSubInstanceData, pFromState);
  }
}

void WStateMachineState_Compound::OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const
{
  auto pData = static_cast<WStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (WUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnExit(ref_instance, pSubInstanceData, pToState);
  }
}

void WStateMachineState_Compound::Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const
{
  auto pData = static_cast<WStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (WUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->Update(ref_instance, pSubInstanceData, deltaTime);
  }
}

WResult WStateMachineState_Compound::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  const WUInt32 uiNumSubStates = m_SubStates.GetCount();
  inout_stream << uiNumSubStates;

  for (auto pSubState : m_SubStates)
  {
    auto pStateType = pSubState->GetDynamicRTTI();
    WTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    W_SUCCEED_OR_RETURN(pSubState->Serialize(inout_stream));
  }

  return W_SUCCESS;
}

WResult WStateMachineState_Compound::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);

  WUInt32 uiNumSubStates = 0;
  inout_stream >> uiNumSubStates;
  m_SubStates.Reserve(uiNumSubStates);

  WStringBuilder sTypeName;
  for (WUInt32 i = 0; i < uiNumSubStates; ++i)
  {
    inout_stream >> sTypeName;
    if (const WRTTI* pType = WRTTI::FindTypeByName(sTypeName))
    {
      WUniquePtr<WStateMachineState> pSubState = pType->GetAllocator()->Allocate<WStateMachineState>();
      W_SUCCEED_OR_RETURN(pSubState->Deserialize(inout_stream));

      m_SubStates.PushBack(pSubState.Release());
    }
    else
    {
      WLog::Error("Unknown state machine state type '{}'", sTypeName);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

bool WStateMachineState_Compound::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubStates.GetArrayPtr(), out_desc);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WStateMachineLogicOperator, 1)
  W_ENUM_CONSTANTS(WStateMachineLogicOperator::And, WStateMachineLogicOperator::Or)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineTransition_BlackboardConditions, 2, WRTTIDefaultAllocator<WStateMachineTransition_BlackboardConditions>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Operator", WStateMachineLogicOperator, m_Operator),
    W_ARRAY_MEMBER_PROPERTY("Conditions", m_Conditions),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineTransition_BlackboardConditions::WStateMachineTransition_BlackboardConditions() = default;
WStateMachineTransition_BlackboardConditions::~WStateMachineTransition_BlackboardConditions() = default;

bool WStateMachineTransition_BlackboardConditions::IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = ref_instance.GetBlackboard();
  if (pBlackboard == nullptr)
    return false;

  const bool bCheckFor = (m_Operator == WStateMachineLogicOperator::Or) ? true : false;
  for (auto& condition : m_Conditions)
  {
    if (condition.IsConditionMet(*pBlackboard) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

WResult WStateMachineTransition_BlackboardConditions::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;
  return inout_stream.WriteArray(m_Conditions);
}

WResult WStateMachineTransition_BlackboardConditions::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Operator;
  return inout_stream.ReadArray(m_Conditions);
}

// Before version 2 each WBlackboardCondition was a reflected class, so the "Conditions" array stored references to
// separate sub-nodes. WBlackboardCondition is now a custom variant type, so the conditions are inlined as values into
// the array. This patch rebuilds the condition objects from the referenced sub-nodes and removes those nodes.
class WStateMachineTransition_BlackboardConditions_1_2 : public WGraphPatch
{
public:
  WStateMachineTransition_BlackboardConditions_1_2()
    : WGraphPatch("WStateMachineTransition_BlackboardConditions", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pConditions = pNode->FindProperty("Conditions");
    if (pConditions == nullptr || !pConditions->m_Value.IsA<WVariantArray>())
      return;

    const WVariantArray oldArray = pConditions->m_Value.Get<WVariantArray>();

    WVariantArray newArray;
    newArray.Reserve(oldArray.GetCount());

    for (const WVariant& element : oldArray)
    {
      if (!element.IsA<WUuid>())
      {
        // Already inlined (e.g. patched before), keep as-is.
        newArray.PushBack(element);
        continue;
      }

      const WUuid guid = element.Get<WUuid>();
      WAbstractObjectNode* pConditionNode = pGraph->GetNode(guid);
      if (pConditionNode == nullptr)
        continue;

      WRttiConverterContext context;
      WRttiConverterReader reader(pGraph, &context);
      void* pObject = reader.CreateObjectFromNode(pConditionNode);
      if (pObject == nullptr)
        continue;

      WVariant value;
      value.MoveTypedObject(pObject, WGetStaticRTTI<WBlackboardCondition>());
      newArray.PushBack(value);

      pGraph->RemoveNode(guid);
    }

    pConditions->m_Value = newArray;
  }
};

WStateMachineTransition_BlackboardConditions_1_2 g_WStateMachineTransition_BlackboardConditions_1_2;

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineTransition_Timeout, 1, WRTTIDefaultAllocator<WStateMachineTransition_Timeout>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Timeout", m_Timeout),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineTransition_Timeout::WStateMachineTransition_Timeout() = default;
WStateMachineTransition_Timeout::~WStateMachineTransition_Timeout() = default;

bool WStateMachineTransition_Timeout::IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetTimeInCurrentState() >= m_Timeout;
}

WResult WStateMachineTransition_Timeout::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Timeout;
  return W_SUCCESS;
}

WResult WStateMachineTransition_Timeout::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Timeout;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineTransition_Compound, 1, WRTTIDefaultAllocator<WStateMachineTransition_Compound>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Operator", WStateMachineLogicOperator, m_Operator),
    W_ARRAY_MEMBER_PROPERTY("SubTransitions", m_SubTransitions)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineTransition_Compound::WStateMachineTransition_Compound() = default;

WStateMachineTransition_Compound::~WStateMachineTransition_Compound()
{
  for (auto pSubState : m_SubTransitions)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

bool WStateMachineTransition_Compound::IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const
{
  auto pData = static_cast<WStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  const bool bCheckFor = (m_Operator == WStateMachineLogicOperator::Or) ? true : false;
  for (WUInt32 i = 0; i < m_SubTransitions.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    if (m_SubTransitions[i]->IsConditionMet(ref_instance, pSubInstanceData) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

WResult WStateMachineTransition_Compound::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;

  const WUInt32 uiNumSubTransitions = m_SubTransitions.GetCount();
  inout_stream << uiNumSubTransitions;

  for (auto pSubTransition : m_SubTransitions)
  {
    auto pStateType = pSubTransition->GetDynamicRTTI();
    WTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    W_SUCCEED_OR_RETURN(pSubTransition->Serialize(inout_stream));
  }

  return W_SUCCESS;
}

WResult WStateMachineTransition_Compound::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Operator;

  WUInt32 uiNumSubTransitions = 0;
  inout_stream >> uiNumSubTransitions;
  m_SubTransitions.Reserve(uiNumSubTransitions);

  WStringBuilder sTypeName;
  for (WUInt32 i = 0; i < uiNumSubTransitions; ++i)
  {
    inout_stream >> sTypeName;
    if (const WRTTI* pType = WRTTI::FindTypeByName(sTypeName))
    {
      WUniquePtr<WStateMachineTransition> pSubTransition = pType->GetAllocator()->Allocate<WStateMachineTransition>();
      W_SUCCEED_OR_RETURN(pSubTransition->Deserialize(inout_stream));

      m_SubTransitions.PushBack(pSubTransition.Release());
    }
    else
    {
      WLog::Error("Unknown state machine state type '{}'", sTypeName);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

bool WStateMachineTransition_Compound::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubTransitions.GetArrayPtr(), out_desc);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineTransition_TransitionEvent, 1, WRTTIDefaultAllocator<WStateMachineTransition_TransitionEvent>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineTransition_TransitionEvent::WStateMachineTransition_TransitionEvent() = default;
WStateMachineTransition_TransitionEvent::~WStateMachineTransition_TransitionEvent() = default;

bool WStateMachineTransition_TransitionEvent::IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetCurrentTransitionEvent() == m_sEventName;
}

WResult WStateMachineTransition_TransitionEvent::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sEventName;
  return W_SUCCESS;
}

WResult WStateMachineTransition_TransitionEvent::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sEventName;
  return W_SUCCESS;
}


W_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineBuiltins);

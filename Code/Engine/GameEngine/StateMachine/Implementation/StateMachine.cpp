#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/Component.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetName),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_OnEnter, In, "StateMachineInstance", In, "FromState")->AddAttributes(new WScriptBaseClassFunctionAttribute(WStateMachineState_ScriptBaseClassFunctions::OnEnter)),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_OnExit, In, "StateMachineInstance", In, "ToState")->AddAttributes(new WScriptBaseClassFunctionAttribute(WStateMachineState_ScriptBaseClassFunctions::OnExit)),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "StateMachineInstance", In, "DeltaTime")->AddAttributes(new WScriptBaseClassFunctionAttribute(WStateMachineState_ScriptBaseClassFunctions::Update)),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_Empty, 1, WRTTIDefaultAllocator<WStateMachineState_Empty>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WHiddenAttribute(),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState::WStateMachineState(WStringView sName)
{
  m_sName.Assign(sName);
}

void WStateMachineState::SetName(WStringView sName)
{
  W_ASSERT_DEV(m_sName.IsEmpty(), "Name can't be changed afterwards");
  m_sName.Assign(sName);
}

void WStateMachineState::OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const
{
}

void WStateMachineState::Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const
{
}

WResult WStateMachineState::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  return W_SUCCESS;
}

WResult WStateMachineState::Deserialize(WStreamReader& inout_stream)
{
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sName;
  return W_SUCCESS;
}

bool WStateMachineState::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  return false;
}

void WStateMachineState::Reflection_OnEnter(WStateMachineInstance* pStateMachineInstance, const WStateMachineState* pFromState)
{
}

void WStateMachineState::Reflection_OnExit(WStateMachineInstance* pStateMachineInstance, const WStateMachineState* pToState)
{
}

void WStateMachineState::Reflection_Update(WStateMachineInstance* pStateMachineInstance, WTime deltaTime)
{
}

WStateMachineState_Empty::WStateMachineState_Empty(WStringView sName)
  : WStateMachineState(sName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineTransition, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WStateMachineTransition::Serialize(WStreamWriter& inout_stream) const
{
  return W_SUCCESS;
}

WResult WStateMachineTransition::Deserialize(WStreamReader& inout_stream)
{
  return W_SUCCESS;
}

bool WStateMachineTransition::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  return false;
}

//////////////////////////////////////////////////////////////////////////

WStateMachineDescription::WStateMachineDescription() = default;
WStateMachineDescription::~WStateMachineDescription() = default;

WUInt32 WStateMachineDescription::AddState(WUniquePtr<WStateMachineState>&& pState)
{
  const WUInt32 uiIndex = m_States.GetCount();

  auto& sStateName = pState->GetNameHashed();
  if (sStateName.IsEmpty() == false)
  {
    W_VERIFY(m_StateNameToIndexTable.Contains(sStateName) == false, "A state with name '{}' already exists.", sStateName);
    m_StateNameToIndexTable.Insert(sStateName, uiIndex);
  }

  StateContext& stateContext = m_States.ExpandAndGetRef();

  WInstanceDataDesc instanceDataDesc;
  if (pState->GetInstanceDataDesc(instanceDataDesc))
  {
    stateContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  stateContext.m_pState = std::move(pState);

  return uiIndex;
}

void WStateMachineDescription::AddTransition(WUInt32 uiFromStateIndex, WUInt32 uiToStateIndex, WUniquePtr<WStateMachineTransition>&& pTransistion)
{
  W_ASSERT_DEV(uiFromStateIndex != uiToStateIndex, "Can't add a transition to itself");

  TransitionArray* pTransitions = nullptr;
  if (uiFromStateIndex == WInvalidIndex)
  {
    pTransitions = &m_FromAnyTransitions;
  }
  else
  {
    W_ASSERT_DEV(uiFromStateIndex < m_States.GetCount(), "Invalid from state index {}", uiFromStateIndex);
    pTransitions = &m_States[uiFromStateIndex].m_Transitions;
  }

  W_ASSERT_DEV(uiToStateIndex < m_States.GetCount(), "Invalid to state index {}", uiToStateIndex);

  TransitionContext& transitionContext = pTransitions->ExpandAndGetRef();

  WInstanceDataDesc instanceDataDesc;
  if (pTransistion->GetInstanceDataDesc(instanceDataDesc))
  {
    transitionContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  transitionContext.m_pTransition = std::move(pTransistion);
  transitionContext.m_uiToStateIndex = uiToStateIndex;
}

constexpr WTypeVersion s_StateMachineDescriptionVersion = 1;

WResult WStateMachineDescription::Serialize(WStreamWriter& ref_originalStream) const
{
  ref_originalStream.WriteVersion(s_StateMachineDescriptionVersion);

  WStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_originalStream);
  WTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  WUInt32 uiNumTransitions = m_FromAnyTransitions.GetCount();

  // states
  {
    const WUInt32 uiNumStates = m_States.GetCount();
    stream << uiNumStates;

    for (auto& stateContext : m_States)
    {
      auto pStateType = stateContext.m_pState->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pStateType);

      stream << pStateType->GetTypeName();
      W_SUCCEED_OR_RETURN(stateContext.m_pState->Serialize(stream));

      uiNumTransitions += stateContext.m_Transitions.GetCount();
    }
  }

  // transitions
  {
    stream << uiNumTransitions;

    auto SerializeTransitions = [&](const TransitionArray& transitions, WUInt32 uiFromStateIndex) -> WResult
    {
      for (auto& transitionContext : transitions)
      {
        const WUInt32 uiToStateIndex = transitionContext.m_uiToStateIndex;

        stream << uiFromStateIndex;
        stream << uiToStateIndex;

        auto pTransitionType = transitionContext.m_pTransition->GetDynamicRTTI();
        typeVersionWriteContext.AddType(pTransitionType);

        stream << pTransitionType->GetTypeName();
        W_SUCCEED_OR_RETURN(transitionContext.m_pTransition->Serialize(stream));
      }

      return W_SUCCESS;
    };

    W_SUCCEED_OR_RETURN(SerializeTransitions(m_FromAnyTransitions, WInvalidIndex));

    for (WUInt32 uiFromStateIndex = 0; uiFromStateIndex < m_States.GetCount(); ++uiFromStateIndex)
    {
      auto& transitions = m_States[uiFromStateIndex].m_Transitions;

      W_SUCCEED_OR_RETURN(SerializeTransitions(transitions, uiFromStateIndex));
    }
  }

  W_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  W_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return W_SUCCESS;
}

WResult WStateMachineDescription::Deserialize(WStreamReader& inout_stream)
{
  const auto uiVersion = inout_stream.ReadVersion(s_StateMachineDescriptionVersion);
  W_IGNORE_UNUSED(uiVersion);

  WStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  WTypeVersionReadContext typeVersionReadContext(inout_stream);

  WStringBuilder sTypeName;

  // states
  {
    WUInt32 uiNumStates = 0;
    inout_stream >> uiNumStates;

    for (WUInt32 i = 0; i < uiNumStates; ++i)
    {
      inout_stream >> sTypeName;
      if (const WRTTI* pType = WRTTI::FindTypeByName(sTypeName))
      {
        WUniquePtr<WStateMachineState> pState = pType->GetAllocator()->Allocate<WStateMachineState>();
        W_SUCCEED_OR_RETURN(pState->Deserialize(inout_stream));

        W_VERIFY(AddState(std::move(pState)) == i, "Implementation error");
      }
      else
      {
        WLog::Error("Unknown state machine state type '{}'", sTypeName);
        return W_FAILURE;
      }
    }
  }

  // transitions
  {
    WUInt32 uiNumTransitions = 0;
    inout_stream >> uiNumTransitions;

    for (WUInt32 i = 0; i < uiNumTransitions; ++i)
    {
      WUInt32 uiFromStateIndex = 0;
      WUInt32 uiToStateIndex = 0;

      inout_stream >> uiFromStateIndex;
      inout_stream >> uiToStateIndex;

      inout_stream >> sTypeName;
      if (const WRTTI* pType = WRTTI::FindTypeByName(sTypeName))
      {
        WUniquePtr<WStateMachineTransition> pTransition = pType->GetAllocator()->Allocate<WStateMachineTransition>();
        W_SUCCEED_OR_RETURN(pTransition->Deserialize(inout_stream));

        AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
      }
      else
      {
        WLog::Error("Unknown state machine transition type '{}'", sTypeName);
        return W_FAILURE;
      }
    }
  }

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WStateMachineInstance, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetState, In, "StateName"),
    W_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    W_SCRIPT_FUNCTION_PROPERTY(GetTimeInCurrentState),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwnerComponent),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetBlackboard),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WStateMachineInstance::WStateMachineInstance(WReflectedClass& ref_owner, const WSharedPtr<const WStateMachineDescription>& pDescription /*= nullptr*/)
  : m_Owner(ref_owner)
  , m_pDescription(pDescription)
{
  if (pDescription != nullptr)
  {
    m_InstanceData = pDescription->m_InstanceDataAllocator.AllocateAndConstruct();
  }
}

WStateMachineInstance::~WStateMachineInstance()
{
  ExitCurrentState(nullptr);

  m_pCurrentState = nullptr;
  m_uiCurrentStateIndex = WInvalidIndex;

  if (m_pDescription != nullptr)
  {
    m_pDescription->m_InstanceDataAllocator.DestructAndDeallocate(m_InstanceData);
  }
}

WResult WStateMachineInstance::SetState(WStateMachineState* pState)
{
  if (m_pCurrentState == pState)
    return W_SUCCESS;

  if (pState != nullptr && m_pDescription != nullptr)
  {
    return SetState(pState->GetNameHashed());
  }

  const auto pFromState = m_pCurrentState;
  const auto pToState = pState;

  ExitCurrentState(pToState);

  m_pCurrentState = pState;
  m_uiCurrentStateIndex = WInvalidIndex;
  m_pCurrentTransitions = nullptr;

  EnterCurrentState(pFromState);

  return W_SUCCESS;
}

WResult WStateMachineInstance::SetState(const WHashedString& sStateName)
{
  W_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by name");

  WUInt32 uiStateIndex = 0;
  if (m_pDescription->m_StateNameToIndexTable.TryGetValue(sStateName, uiStateIndex))
  {
    SetStateInternal(uiStateIndex);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WStateMachineInstance::SetState(WUInt32 uiStateIndex)
{
  W_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by index");

  if (uiStateIndex < m_pDescription->m_States.GetCount())
  {
    SetStateInternal(uiStateIndex);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WStateMachineInstance::SetStateOrFallback(const WHashedString& sStateName, WUInt32 uiFallbackStateIndex /*= 0*/)
{
  if (SetState(sStateName).Failed())
  {
    return SetState(uiFallbackStateIndex);
  }

  return W_SUCCESS;
}

void WStateMachineInstance::Update(WTime deltaTime)
{
  WUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != WInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->Update(*this, pInstanceData, deltaTime);
  }

  m_TimeInCurrentState += deltaTime;
}

WWorld* WStateMachineInstance::GetOwnerWorld()
{
  if (auto pComponent = WDynamicCast<WComponent*>(&m_Owner))
  {
    return pComponent->GetWorld();
  }

  return nullptr;
}

void WStateMachineInstance::SetBlackboard(const WSharedPtr<WBlackboard>& pBlackboard)
{
  m_pBlackboard = pBlackboard;
}

void WStateMachineInstance::FireTransitionEvent(WStringView sEvent)
{
  m_sCurrentTransitionEvent = sEvent;

  WUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != WInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  m_sCurrentTransitionEvent = {};
}

bool WStateMachineInstance::Reflection_SetState(const WHashedString& sStateName)
{
  return SetState(sStateName).Succeeded();
}

WComponent* WStateMachineInstance::Reflection_GetOwnerComponent() const
{
  return WDynamicCast<WComponent*>(&m_Owner);
}

void WStateMachineInstance::SetStateInternal(WUInt32 uiStateIndex)
{
  if (m_uiCurrentStateIndex == uiStateIndex)
    return;

  const auto& stateContext = m_pDescription->m_States[uiStateIndex];
  const auto pFromState = m_pCurrentState;
  const auto pToState = stateContext.m_pState.Borrow();

  ExitCurrentState(pToState);

  m_pCurrentState = pToState;
  m_uiCurrentStateIndex = uiStateIndex;
  m_pCurrentTransitions = &stateContext.m_Transitions;

  EnterCurrentState(pFromState);
}

void WStateMachineInstance::EnterCurrentState(const WStateMachineState* pFromState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnEnter(*this, pInstanceData, pFromState);

    m_TimeInCurrentState = WTime::MakeZero();
  }
}

void WStateMachineInstance::ExitCurrentState(const WStateMachineState* pToState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnExit(*this, pInstanceData, pToState);
  }
}

WUInt32 WStateMachineInstance::FindNewStateToTransitionTo()
{
  if (m_pCurrentTransitions != nullptr)
  {
    for (auto& transitionContext : *m_pCurrentTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  if (m_pDescription != nullptr)
  {
    for (auto& transitionContext : m_pDescription->m_FromAnyTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  return WInvalidIndex;
}


W_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachine);

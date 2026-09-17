#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

namespace
{
  class TestState : public WStateMachineState
  {
    W_ADD_DYNAMIC_REFLECTION(TestState, WStateMachineState);

  public:
    TestState(WStringView sName = WStringView())
      : WStateMachineState(sName)
    {
    }

    virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiEnterCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiExitCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct Counter
    {
      WUInt32 m_uiEnterCounter = 0;
      WUInt32 m_uiExitCounter = 0;
    };

    mutable WHashTable<WStateMachineInstance*, Counter> m_CounterTable;

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      Counter m_Counter;

      static WUInt32 s_uiConstructionCounter;
      static WUInt32 s_uiDestructionCounter;
    };
  };

  WUInt32 TestState::InstanceData::s_uiConstructionCounter = 0;
  WUInt32 TestState::InstanceData::s_uiDestructionCounter = 0;

  // clang-format off
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(TestState, 1, WRTTIDefaultAllocator<TestState>)
  W_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestTransition : public WStateMachineTransition
  {
  public:
    bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_uiConditionCounter++;

      return pData->m_uiConditionCounter > 1;
    }

    bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      WUInt32 m_uiConditionCounter;

      static WUInt32 s_uiConstructionCounter;
      static WUInt32 s_uiDestructionCounter;
    };
  };

  WUInt32 TestTransition::InstanceData::s_uiConstructionCounter = 0;
  WUInt32 TestTransition::InstanceData::s_uiDestructionCounter = 0;

  static void ResetCounter()
  {
    TestState::InstanceData::s_uiConstructionCounter = 0;
    TestState::InstanceData::s_uiDestructionCounter = 0;
    TestTransition::InstanceData::s_uiConstructionCounter = 0;
    TestTransition::InstanceData::s_uiDestructionCounter = 0;
  }

  static WTime s_TimeStep = WTime::MakeFromMilliseconds(10);

} // namespace

void WGameEngineTestStateMachine::RunBuiltinsTest()
{
  WReflectedClass fakeOwner;

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple States")
  {
    ResetCounter();

    WSharedPtr<WStateMachineDescription> pDesc = W_DEFAULT_NEW(WStateMachineDescription);

    auto pStateA = W_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = W_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = W_DEFAULT_NEW(TestTransition);
    pDesc->AddTransition(1, 0, pTransition);

    WStateMachineInstance* pInstance = nullptr;
    {
      WStateMachineInstance sm(fakeOwner, pDesc);
      W_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 2);
      W_TEST_INT(TestTransition::InstanceData::s_uiConstructionCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      WHashedString sStateName; // intentionally left empty to go to fallback state (state with index 0 -> state "A")
      W_TEST_BOOL(sm.SetStateOrFallback(sStateName).Succeeded());
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      W_TEST_BOOL(sm.SetState(pStateB).Succeeded());
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sStateName.Assign("C");
      W_TEST_BOOL(sm.SetState(sStateName).Failed());

      // no transition yet
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // go back to "A"
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 2);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);

      pInstance = &sm; // will be dead after this line but we only need the pointer
    }

    W_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 2);
    W_TEST_INT(TestTransition::InstanceData::s_uiDestructionCounter, 1);
    W_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiEnterCounter, 2);
    W_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiExitCounter, 2);
    W_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiEnterCounter, 1);
    W_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiExitCounter, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Blackboard Transition")
  {
    ResetCounter();

    WSharedPtr<WStateMachineDescription> pDesc = W_DEFAULT_NEW(WStateMachineDescription);

    auto pStateA = W_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = W_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pStateC = W_DEFAULT_NEW(TestState, "C");
    pDesc->AddState(pStateC);

    WHashedString sTestVal = WMakeHashedString("TestVal");
    WHashedString sTestVal2 = WMakeHashedString("TestVal2");

    {
      auto pTransition = W_DEFAULT_NEW(WStateMachineTransition_BlackboardConditions);
      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 2;
      cond.m_Operator = WComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 10;
      cond2.m_Operator = WComparisonOperator::Equal;

      pDesc->AddTransition(0, 1, pTransition);
    }

    {
      auto pTransition = W_DEFAULT_NEW(WStateMachineTransition_BlackboardConditions);
      pTransition->m_Operator = WStateMachineLogicOperator::Or;

      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 3;
      cond.m_Operator = WComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 20;
      cond2.m_Operator = WComparisonOperator::Equal;

      pDesc->AddTransition(1, 2, pTransition);
    }

    {
      WSharedPtr<WBlackboard> pBlackboard = WBlackboard::Create("TestBB");
      pBlackboard->SetEntryValue(sTestVal, 2);
      pBlackboard->SetEntryValue(sTestVal2, 0);

      WStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      W_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      // no transition yet since only part of the conditions is true
      pBlackboard->SetEntryValue(sTestVal, 3);
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to B
      pBlackboard->SetEntryValue(sTestVal2, 10);
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to C, only part of the condition needed because of 'OR' operator
      pBlackboard->SetEntryValue(sTestVal2, 20);
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Timeout Transition")
  {
    ResetCounter();

    WSharedPtr<WStateMachineDescription> pDesc = W_DEFAULT_NEW(WStateMachineDescription);

    auto pStateA = W_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = W_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = W_DEFAULT_NEW(WStateMachineTransition_Timeout);
    pTransition->m_Timeout = WTime::MakeFromMilliseconds(5);
    pDesc->AddTransition(0, 1, pTransition);

    {
      WStateMachineInstance sm(fakeOwner, pDesc);
      W_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sm.Update(s_TimeStep);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compounds")
  {
    ResetCounter();

    WSharedPtr<WStateMachineDescription> pDesc = W_DEFAULT_NEW(WStateMachineDescription);

    auto pCompoundState = W_DEFAULT_NEW(WStateMachineState_Compound, "A");
    {
      auto pAllocator = WGetStaticRTTI<TestState>()->GetAllocator();
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
    }
    pDesc->AddState(pCompoundState);

    auto pStateB = W_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    WHashedString sTestVal = WMakeHashedString("TestVal");

    {
      auto pCompoundTransition = W_DEFAULT_NEW(WStateMachineTransition_Compound);

      {
        auto pAllocator = WGetStaticRTTI<WStateMachineTransition_BlackboardConditions>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<WStateMachineTransition_BlackboardConditions>();

        auto& cond = pSubTransition->m_Conditions.ExpandAndGetRef();
        cond.m_sEntryName = sTestVal;
        cond.m_fComparisonValue = 2;
        cond.m_Operator = WComparisonOperator::Greater;

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      {
        auto pAllocator = WGetStaticRTTI<WStateMachineTransition_Timeout>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<WStateMachineTransition_Timeout>();
        pSubTransition->m_Timeout = WTime::MakeFromMilliseconds(5);

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      pDesc->AddTransition(0, 1, pCompoundTransition);
    }

    {
      WSharedPtr<WBlackboard> pBlackboard = WBlackboard::Create("TestBB");
      pBlackboard->SetEntryValue(sTestVal, 2);

      WStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      W_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 1); // Compound instance data not constructed yet

      W_TEST_BOOL(sm.SetState(pCompoundState).Succeeded());
      W_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 3);
      W_TEST_INT(WStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(WStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // no transition yet because timeout is not reached yet
      pBlackboard->SetEntryValue(sTestVal, 3);
      sm.Update(s_TimeStep);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // all conditions met, transition to B
      sm.Update(s_TimeStep);
      W_TEST_INT(WStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(WStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiExitCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      W_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }

    W_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 3);
  }
}

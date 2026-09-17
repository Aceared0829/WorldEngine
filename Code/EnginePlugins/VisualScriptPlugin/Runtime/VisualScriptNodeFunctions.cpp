#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Containers/IterateBits.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecResult = WVisualScriptGraphDescription::ExecResult;
using ExecuteFunctionGetter = WVisualScriptGraphDescription::ExecuteFunction (*)(WVisualScriptDataType::Enum dataType);

#define MAKE_EXEC_FUNC_GETTER(funcName)                                                                                  \
  WVisualScriptGraphDescription::ExecuteFunction W_PP_CONCAT(funcName, _Getter)(WVisualScriptDataType::Enum dataType) \
  {                                                                                                                      \
    static WVisualScriptGraphDescription::ExecuteFunction functionTable[] = {                                           \
      nullptr, /* Invalid*/                                                                                              \
      &funcName<bool>,                                                                                                   \
      &funcName<WUInt8>,                                                                                                \
      &funcName<WInt32>,                                                                                                \
      &funcName<WInt64>,                                                                                                \
      &funcName<float>,                                                                                                  \
      &funcName<double>,                                                                                                 \
      &funcName<WColor>,                                                                                                \
      &funcName<WVec2>,                                                                                                 \
      &funcName<WVec3>,                                                                                                 \
      &funcName<WVec4>,                                                                                                 \
      &funcName<WQuat>,                                                                                                 \
      &funcName<WTransform>,                                                                                            \
      &funcName<WTime>,                                                                                                 \
      &funcName<WAngle>,                                                                                                \
      &funcName<WString>,                                                                                               \
      &funcName<WHashedString>,                                                                                         \
      &funcName<WGameObjectHandle>,                                                                                     \
      &funcName<WComponentHandle>,                                                                                      \
      &funcName<WTypedPointer>,                                                                                         \
      &funcName<WVariant>,                                                                                              \
      &funcName<WVariantArray>,                                                                                         \
      &funcName<WVariantDictionary>,                                                                                    \
      &funcName<WScriptCoroutineHandle>,                                                                                \
    };                                                                                                                   \
                                                                                                                         \
    static_assert(W_ARRAY_SIZE(functionTable) == WVisualScriptDataType::Count);                                        \
    if (dataType >= 0 && dataType < W_ARRAY_SIZE(functionTable))                                                        \
      return functionTable[dataType];                                                                                    \
                                                                                                                         \
    WLog::Error("Invalid data type for deducted type {}. Script needs re-transform.", dataType);                        \
    return nullptr;                                                                                                      \
  }

template <typename T>
WStringView GetTypeName()
{
  if constexpr (std::is_same_v<T, WTypedPointer>)
  {
    return "WTypePointer";
  }
  else
  {
    return WGetStaticRTTI<T>()->GetTypeName();
  }
}

namespace
{
  static W_FORCE_INLINE WResult FillFunctionArgs(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node, const WAbstractFunctionProperty* pFunction, WUInt32 uiInputArgsMask, WUInt32 uiStartSlot, WDynamicArray<WVariant>& out_args)
  {
    const WUInt32 uiArgCount = pFunction->GetArgumentCount();

    WUInt32 uiInputSlot = uiStartSlot;
    for (WUInt32 i = 0; i < uiArgCount; ++i)
    {
      const WRTTI* pArgType = pFunction->GetArgumentType(i);
      if ((uiInputArgsMask & W_BIT(i)) != 0)
      {
        out_args.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(uiInputSlot), pArgType));
        ++uiInputSlot;
      }
      else
      {
        out_args.PushBack(WReflectionUtils::GetDefaultVariantFromType(pArgType));
      }
    }

    return W_SUCCESS;
  }

  static W_FORCE_INLINE WScriptWorldModule* GetScriptModule(WVisualScriptExecutionContext& inout_context)
  {
    WWorld* pWorld = inout_context.GetInstance().GetWorld();
    if (pWorld == nullptr)
    {
      WLog::Error("Visual script coroutines need a script instance with a valid WWorld");
      return nullptr;
    }

    return pWorld->GetOrCreateModule<WScriptWorldModule>();
  }

  static ExecResult NodeFunction_ReflectedFunction(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndFunction>();
    W_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == WPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const WAbstractFunctionProperty*>(userData.m_pProperty);

    WTypedPointer pInstance;
    WUInt32 uiInputSlot = 0;

    if (pFunction->GetFunctionType() == WFunctionType::Member)
    {
      pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        WLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return ExecResult::Error();
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        WLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return ExecResult::Error();
      }

      ++uiInputSlot;
    }

    WTempHybridArray<WVariant, 8> args;
    if (FillFunctionArgs(inout_context, node, pFunction, userData.m_uiInputArgsMask, uiInputSlot, args).Failed())
    {
      return ExecResult::Error();
    }

    WVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    WUInt32 uiOutputSlot = 0;
    if (pFunction->GetReturnType() != nullptr)
    {
      inout_context.SetDataFromVariant(node.GetOutputDataOffset(0), returnValue);
      ++uiOutputSlot;
    }

    for (WUInt32 uiArgIndex : WIterateBitIndices(userData.m_uiOutputArgsMask))
    {
      inout_context.SetDataFromVariant(node.GetOutputDataOffset(uiOutputSlot), args[uiArgIndex]);
      ++uiOutputSlot;
    }

    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_GetReflectedProperty(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    WTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      WLog::Error("Visual script get property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      WLog::Error("Visual script get property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == WPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const WAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                    std::is_same_v<T, WComponentHandle> ||
                    std::is_same_v<T, WTypedPointer>)
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        if (pProperty->GetSpecificType() == WGetStaticRTTI<T>())
        {
          T value;
          pMemberProperty->GetValuePtr(pInstance.m_pObject, &value);
          inout_context.SetData(node.GetOutputDataOffset(0), value);
        }
        else
        {
          WVariant value = WReflectionUtils::GetMemberPropertyValue(pMemberProperty, pInstance.m_pObject);
          inout_context.SetDataFromVariant(node.GetOutputDataOffset(0), value);
        }
      }
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_GetReflectedProperty);

  template <typename T>
  static ExecResult NodeFunction_SetReflectedProperty(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    WTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      WLog::Error("Visual script set property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      WLog::Error("Visual script set property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == WPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const WAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                    std::is_same_v<T, WComponentHandle> ||
                    std::is_same_v<T, WTypedPointer>)
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        if (pProperty->GetSpecificType() == WGetStaticRTTI<T>())
        {
          const T& value = inout_context.GetData<T>(node.GetInputDataOffset(1));
          pMemberProperty->SetValuePtr(pInstance.m_pObject, &value);
        }
        else
        {
          WVariant value = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), pProperty->GetSpecificType());
          WReflectionUtils::SetMemberPropertyValue(pMemberProperty, pInstance.m_pObject, value);
        }
      }
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_SetReflectedProperty);

  static ExecResult NodeFunction_InplaceCoroutine(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      auto pModule = GetScriptModule(inout_context);
      if (pModule == nullptr)
        return ExecResult::Error();

      auto& userData = node.GetUserData<NodeUserData_TypeAndFunction>();
      pModule->CreateCoroutine(userData.m_pType, userData.m_pType->GetTypeName(), inout_context.GetInstance(), WScriptCoroutineCreationMode::AllowOverlap, pCoroutine);

      W_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == WPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
      auto pFunction = static_cast<const WAbstractFunctionProperty*>(userData.m_pProperty);

      WTempHybridArray<WVariant, 8> args;
      if (FillFunctionArgs(inout_context, node, pFunction, userData.m_uiInputArgsMask, 0, args).Failed())
      {
        return ExecResult::Error();
      }

      pCoroutine->StartWithVarargs(args);

      inout_context.SetCurrentCoroutine(pCoroutine);
    }

    auto result = pCoroutine->Update(inout_context.GetDeltaTimeSinceLastExecution());
    if (result.m_State == WScriptCoroutine::Result::State::Running)
    {
      return ExecResult::ContinueLater(result.m_MaxDelay);
    }

    WWorld* pWorld = inout_context.GetInstance().GetWorld();
    auto pModule = pWorld->GetOrCreateModule<WScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(pCoroutine->GetHandle());
    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(result.m_State == WScriptCoroutine::Result::State::Completed ? 0 : 1);
  }

  static ExecResult NodeFunction_GetScriptOwner(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WWorld* pWorld = inout_context.GetInstance().GetWorld();
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pWorld, WGetStaticRTTI<WWorld>());

    WReflectedClass& owner = inout_context.GetInstance().GetOwner();
    if (auto pComponent = WDynamicCast<WComponent*>(&owner))
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      inout_context.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_SendMessage(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperties>();
    auto targetObjectDataOffset = node.GetInputDataOffset(0);
    auto targetComponentDataOffset = node.GetInputDataOffset(1);

    auto pTargetObject = targetObjectDataOffset.IsValid() ? static_cast<WGameObject*>(inout_context.GetPointerData(targetObjectDataOffset).m_pObject) : nullptr;
    auto pTargetComponent = targetComponentDataOffset.IsValid() ? static_cast<WComponent*>(inout_context.GetPointerData(targetComponentDataOffset).m_pObject) : nullptr;
    if (pTargetObject == nullptr && pTargetComponent == nullptr)
    {
      WLog::Error("Visual script send '{}': Invalid target game object and component.", userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    auto mode = static_cast<WVisualScriptSendMessageMode::Enum>(inout_context.GetData<WInt64>(node.GetInputDataOffset(2)));
    WTime delay = inout_context.GetData<WTime>(node.GetInputDataOffset(3));

    WScriptComponent* pSenderComponent = nullptr;
    if (mode == WVisualScriptSendMessageMode::Event)
    {
      pSenderComponent = WDynamicCast<WScriptComponent*>(&inout_context.GetInstance().GetOwner());
    }

    const WUInt32 uiStartSlot = 4;

    WUniquePtr<WMessage> pMessage = userData.m_pType->GetAllocator()->Allocate<WMessage>(WTempAllocator::Get());
    for (WUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
    {
      auto pProp = userData.m_Properties[i];
      const WRTTI* pPropType = pProp->GetSpecificType();
      WVariant value = inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pPropType);

      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        WReflectionUtils::SetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pMessage.Borrow(), value);
      }
      else
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
    }

    bool bWriteOutputs = false;
    if (pTargetComponent != nullptr)
    {
      if (delay.IsPositive())
      {
        pTargetComponent->PostMessage(*pMessage, delay);
      }
      else
      {
        bWriteOutputs = pTargetComponent->SendMessage(*pMessage);
      }
    }
    else if (pTargetObject != nullptr)
    {
      if (delay.IsPositive())
      {
        if (mode == WVisualScriptSendMessageMode::Direct)
          pTargetObject->PostMessage(*pMessage, delay);
        else if (mode == WVisualScriptSendMessageMode::Recursive)
          pTargetObject->PostMessageRecursive(*pMessage, delay);
        else
          pTargetObject->PostEventMessage(*pMessage, pSenderComponent, delay);
      }
      else
      {
        if (mode == WVisualScriptSendMessageMode::Direct)
          bWriteOutputs = pTargetObject->SendMessage(*pMessage);
        else if (mode == WVisualScriptSendMessageMode::Recursive)
          bWriteOutputs = pTargetObject->SendMessageRecursive(*pMessage);
        else
          bWriteOutputs = pTargetObject->SendEventMessage(*pMessage, pSenderComponent);
      }
    }

    if (bWriteOutputs)
    {
      for (WUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
      {
        auto dataOffset = node.GetOutputDataOffset(i);
        if (dataOffset.IsValid() == false)
          continue;

        auto pProp = userData.m_Properties[i];
        WVariant value;

        if (pProp->GetCategory() == WPropertyCategory::Member)
        {
          value = WReflectionUtils::GetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pMessage.Borrow());
        }
        else
        {
          W_ASSERT_NOT_IMPLEMENTED;
        }

        inout_context.SetDataFromVariant(dataOffset, value);
      }
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_SetVariable(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WGameObjectHandle>)
    {
      WTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), static_cast<WGameObject*>(ptr.m_pObject), WGetStaticRTTI<WGameObject>());
    }
    else if constexpr (std::is_same_v<T, WComponentHandle>)
    {
      WTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), static_cast<WComponent*>(ptr.m_pObject), WGetStaticRTTI<WComponent>());
    }
    else if constexpr (std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), ptr.m_pObject, ptr.m_pType);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<T>(node.GetInputDataOffset(0)));
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_SetVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IncVariable(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), ++a);
    }
    else
    {
      WLog::Error("Increment is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IncVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_DecVariable(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), --a);
    }
    else
    {
      WLog::Error("Decrement is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_DecVariable);

  static ExecResult NodeFunction_Builtin_Branch(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    return ExecResult::RunNext(bCondition ? 0 : 1);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Switch(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WInt64 iValue = 0;
    if constexpr (std::is_same_v<T, WInt64>)
    {
      iValue = inout_context.GetData<WInt64>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      iValue = inout_context.GetData<WHashedString>(node.GetInputDataOffset(0)).GetHash();
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }

    auto& userData = node.GetUserData<NodeUserData_Switch>();
    for (WUInt32 i = 0; i < userData.m_uiNumCases; ++i)
    {
      if (iValue == userData.m_Cases[i])
      {
        return ExecResult::RunNext(i);
      }
    }

    return ExecResult::RunNext(userData.m_uiNumCases);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Switch);

  static ExecResult NodeFunction_Builtin_And(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a && b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Or(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a || b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Not(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), !a);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Compare(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Comparison>();
    bool bRes = false;

    if constexpr (std::is_same_v<T, bool> ||
                  std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WVec2> ||
                  std::is_same_v<T, WVec3> ||
                  std::is_same_v<T, WVec4> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle> ||
                  std::is_same_v<T, WString> ||
                  std::is_same_v<T, WHashedString>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      bRes = WComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                       std::is_same_v<T, WComponentHandle> ||
                       std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(0));
      WTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(1));
      bRes = WComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      WVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      WVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);

      if (userData.m_ComparisonOperator == WComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == WComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        WLog::Error("Comparison '{}' is not defined for type '{}'", WArgEnum(userData.m_ComparisonOperator), GetTypeName<T>());
      }
    }
    else if constexpr (std::is_same_v<T, WQuat> ||
                       std::is_same_v<T, WTransform> ||
                       std::is_same_v<T, WVariantArray> ||
                       std::is_same_v<T, WVariantDictionary>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));

      if (userData.m_ComparisonOperator == WComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == WComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        WLog::Error("Comparison '{}' is not defined for type '{}'", WArgEnum(userData.m_ComparisonOperator), GetTypeName<T>());
      }
    }
    else
    {
      WLog::Error("Comparison is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IsValid(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same_v<T, float>)
    {
      bIsValid = WMath::IsFinite(inout_context.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      bIsValid = WMath::IsFinite(inout_context.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, WColor>)
    {
      bIsValid = inout_context.GetData<WColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, WVec2>)
    {
      bIsValid = inout_context.GetData<WVec2>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, WVec3>)
    {
      bIsValid = inout_context.GetData<WVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, WVec4>)
    {
      bIsValid = inout_context.GetData<WVec4>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, WQuat>)
    {
      bIsValid = inout_context.GetData<WQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, WString>)
    {
      bIsValid = inout_context.GetData<WString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      bIsValid = inout_context.GetData<WHashedString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                       std::is_same_v<T, WComponentHandle> ||
                       std::is_same_v<T, WTypedPointer>)
    {
      bIsValid = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      bIsValid = inout_context.GetData<WVariant>(dataOffset).IsValid();
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bIsValid);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Select(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));

    if constexpr (std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(1));
      WTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(2));
      WTypedPointer res = bCondition ? a : b;
      inout_context.SetPointerData(node.GetOutputDataOffset(0), res.m_pObject, res.m_pType);
    }
    else
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(1));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(2));
      inout_context.SetData(node.GetOutputDataOffset(0), bCondition ? a : b);
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Select);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_Add(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WVec2> ||
                  std::is_same_v<T, WVec3> ||
                  std::is_same_v<T, WVec4> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else if constexpr (std::is_same_v<T, WString>)
    {
      auto& a = inout_context.GetData<WString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<WString>(node.GetInputDataOffset(1));

      WStringBuilder s;
      s.Set(a, b);

      inout_context.SetData(node.GetOutputDataOffset(0), WString(s.GetView()));
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      auto& a = inout_context.GetData<WHashedString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<WHashedString>(node.GetInputDataOffset(1));

      WStringBuilder s;
      s.Set(a, b);
      WHashedString sHashed;
      sHashed.Assign(s);

      inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      WVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      WVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a + b);
    }
    else
    {
      WLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Sub(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WVec2> ||
                  std::is_same_v<T, WVec3> ||
                  std::is_same_v<T, WVec4> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      WVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      WVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a - b);
    }
    else
    {
      WLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Mul(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WQuat>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same_v<T, WAngle>)
    {
      const WAngle& a = inout_context.GetData<WAngle>(node.GetInputDataOffset(0));
      const WAngle& b = inout_context.GetData<WAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), WAngle(a * b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      WVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      WVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a * b);
    }
    else
    {
      WLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Div(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same_v<T, WAngle>)
    {
      const WAngle& a = inout_context.GetData<WAngle>(node.GetInputDataOffset(0));
      const WAngle& b = inout_context.GetData<WAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), WAngle(a / b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      WVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      WVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a / b);
    }
    else
    {
      WLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Mod(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a % b));
    }
    else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(WMath::Mod(a, b)));
    }
    else
    {
      WLog::Error("Modulo is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mod);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Min(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), WMath::Min(a, b));
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMin(b));
    }
    else
    {
      WLog::Error("Min is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Min);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Max(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), WMath::Max(a, b));
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMax(b));
    }
    else
    {
      WLog::Error("Max is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Max);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Clamp(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt32> ||
                  std::is_same_v<T, WInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      const T& x = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(1));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(2));
      inout_context.SetData(node.GetOutputDataOffset(0), WMath::Clamp(x, a, b));
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4>)
    {
      const T& x = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(1));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(2));
      inout_context.SetData(node.GetOutputDataOffset(0), x.CompClamp(a, b));
    }
    else
    {
      WLog::Error("Clamp is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Clamp);

  static ExecResult NodeFunction_Builtin_Expression(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    static WHashedString sStream = WMakeHashedString("VsStream");

    int iDummy = 0;
    WTempHybridArray<WProcessingStream, 8> inputStreams;
    for (WUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      WTypedPointer ptr;
      if (dataOffset.IsConstant())
      {
        ptr.m_pObject = &iDummy;
      }
      else
      {
        ptr = inout_context.GetPointerData(dataOffset);
      }

      const WUInt32 uiDataSize = WVisualScriptDataType::GetStorageSize(dataOffset.GetType());
      auto streamDataType = WVisualScriptDataType::GetStreamDataType(dataOffset.GetType());

      inputStreams.PushBack(WProcessingStream(sStream, WMakeArrayPtr(static_cast<WUInt8*>(ptr.m_pObject), uiDataSize), streamDataType));
    }

    auto& userData = node.GetUserData<NodeUserData_Expression>();

    WUInt8 dummyOutput[sizeof(WVec4)];
    WTempHybridArray<WProcessingStream, 8> outputStreams;
    for (WUInt32 i = 0; i < node.m_NumOutputDataOffsets; ++i)
    {
      auto dataOffset = node.GetOutputDataOffset(i);
      if (dataOffset.IsValid())
      {
        WTypedPointer ptr = inout_context.GetPointerData(dataOffset);

        const WUInt32 uiDataSize = WVisualScriptDataType::GetStorageSize(dataOffset.GetType());
        auto streamDataType = WVisualScriptDataType::GetStreamDataType(dataOffset.GetType());

        outputStreams.PushBack(WProcessingStream(sStream, WMakeArrayPtr(static_cast<WUInt8*>(ptr.m_pObject), uiDataSize), streamDataType));
      }
      else
      {
        auto& outputStreamDesc = userData.m_ByteCode.GetOutputs()[i];
        outputStreams.PushBack(WProcessingStream(sStream, WMakeArrayPtr(dummyOutput), outputStreamDesc.m_DataType));
      }
    }

    if (pModule->GetSharedExpressionVM().Execute(userData.m_ByteCode, inputStreams, outputStreams, 1, WExpression::GlobalData(), WExpressionVM::Flags::ScalarizeStreams).Failed())
    {
      WLog::Error("Visual script expression execution failed");
      return ExecResult::Error();
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToBool(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same_v<T, bool>)
    {
      bRes = inout_context.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, WUInt8> ||
                       std::is_same_v<T, WInt32> ||
                       std::is_same_v<T, WInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      bRes = inout_context.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                       std::is_same_v<T, WComponentHandle> ||
                       std::is_same_v<T, WTypedPointer>)
    {
      bRes = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      bRes = inout_context.GetData<WVariant>(dataOffset).ConvertTo<bool>();
    }
    else
    {
      WLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  W_FORCE_INLINE static ExecResult NodeFunction_Builtin_ToNumber(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same_v<T, bool>)
    {
      res = inout_context.GetData<T>(dataOffset) ? NumberType(1) : NumberType(0);
    }
    else if constexpr (std::is_same_v<T, WUInt8> ||
                       std::is_same_v<T, WInt32> ||
                       std::is_same_v<T, WInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      res = static_cast<NumberType>(inout_context.GetData<T>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      res = inout_context.GetData<WVariant>(dataOffset).ConvertTo<NumberType>();
    }
    else
    {
      WLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), res);
    return ExecResult::RunNext(0);
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                                 \
  template <typename T>                                                                                                                                           \
  static ExecResult W_PP_CONCAT(NodeFunction_Builtin_To, Name)(WVisualScriptExecutionContext & inout_context, const WVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                                               \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(inout_context, node, #Name);                                                                              \
  }

  MAKE_TONUMBER_EXEC_FUNC(WUInt8, Byte);
  MAKE_TONUMBER_EXEC_FUNC(WInt32, Int);
  MAKE_TONUMBER_EXEC_FUNC(WInt64, Int64);
  MAKE_TONUMBER_EXEC_FUNC(float, Float);
  MAKE_TONUMBER_EXEC_FUNC(double, Double);

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToByte);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt64);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToFloat);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToDouble);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToString(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    WStringBuilder sb;
    WStringView s;
    if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                  std::is_same_v<T, WComponentHandle> ||
                  std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer p = inout_context.GetPointerData(dataOffset);
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), WArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, WString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<WString>(dataOffset));
      return ExecResult::RunNext(0);
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<WHashedString>(dataOffset).GetString());
      return ExecResult::RunNext(0);
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<WVariant>(dataOffset).ConvertTo<WString>());
      return ExecResult::RunNext(0);
    }
    else
    {
      s = WConversionUtils::ToString(inout_context.GetData<T>(dataOffset), sb);
    }

    inout_context.SetData(node.GetOutputDataOffset(0), s);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToHashedString(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    WStringBuilder sb;
    WStringView s;
    if constexpr (std::is_same_v<T, WGameObjectHandle> ||
                  std::is_same_v<T, WComponentHandle> ||
                  std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer p = inout_context.GetPointerData(dataOffset);
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), WArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, WString>)
    {
      s = inout_context.GetData<WString>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<WHashedString>(dataOffset));
      return ExecResult::RunNext(0);
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<WVariant>(dataOffset).ConvertTo<WHashedString>());
      return ExecResult::RunNext(0);
    }
    else
    {
      s = WConversionUtils::ToString(inout_context.GetData<T>(dataOffset), sb);
    }

    WHashedString sHashed;
    sHashed.Assign(s);
    inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToHashedString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToVariant(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariant v;
    if constexpr (std::is_same_v<T, WTypedPointer>)
    {
      WTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      v = WVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = inout_context.GetData<T>(node.GetInputDataOffset(0));
    }
    inout_context.SetData(node.GetOutputDataOffset(0), v);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Variant_ConvertTo(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariant& v = inout_context.GetData<WVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same_v<T, WTypedPointer>)
    {
      if (v.IsA<WTypedPointer>())
      {
        WTypedPointer typedPtr = v.Get<WTypedPointer>();
        inout_context.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return ExecResult::RunNext(0);
      }

      inout_context.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return ExecResult::RunNext(1);
    }
    else if constexpr (std::is_same_v<T, WVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), v);
      return ExecResult::RunNext(0);
    }
    else
    {
      WResult conversionResult = W_SUCCESS;
      inout_context.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return ExecResult::RunNext(conversionResult.Succeeded() ? 0 : 1);
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_String_Format(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& sText = inout_context.GetData<WString>(node.GetInputDataOffset(0));

    WTempHybridArray<WString, 12> stringStorage;
    stringStorage.Reserve(node.m_NumInputDataOffsets - 1);
    for (WUInt32 i = 1; i < node.m_NumInputDataOffsets; ++i)
    {
      stringStorage.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(i), nullptr).ConvertTo<WString>());
    }

    WTempHybridArray<WStringView, 12> stringViews;
    stringViews.Reserve(stringStorage.GetCount());
    for (auto& s : stringStorage)
    {
      stringViews.PushBack(s);
    }

    WFormatString fs(sText.GetView());
    WStringBuilder sStorage;
    WStringView sFormatted = fs.BuildFormattedText(sStorage, stringViews.GetData(), stringViews.GetCount());

    inout_context.SetData(node.GetOutputDataOffset(0), sFormatted);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_String_GetCharacterCount(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& sText = inout_context.GetData<WString>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), sText.GetCharacterCount());
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_String_IsEmpty(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& sText = inout_context.GetData<WString>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), sText.IsEmpty());
    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_MakeArray(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (WUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.IsConstant())
      {
        a.PushBack(inout_context.GetDataAsVariant(dataOffset, nullptr));
      }
      else
      {
        a.PushBack(inout_context.GetData<WVariant>(dataOffset));
      }
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_GetElement(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariantArray& a = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      inout_context.SetData(node.GetOutputDataOffset(0), a[iIndex]);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), WVariant());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_SetElement(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      a[iIndex] = inout_context.GetDataAsVariant(node.GetInputDataOffset(2), nullptr);
      return ExecResult::RunNext(0);
    }

    WLog::Error("Visual script Array::SetElement: Index '{}' is out of bounds. Valid range is [0, {}).", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  static ExecResult NodeFunction_Builtin_Array_GetCount(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariantArray& a = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<int>(node.GetOutputDataOffset(0), a.GetCount());

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Clear(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    a.Clear();

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_IsEmpty(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariantArray& a = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<bool>(node.GetOutputDataOffset(0), a.IsEmpty());

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Contains(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariantArray& a = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    inout_context.SetData<bool>(node.GetOutputDataOffset(0), a.Contains(element));

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_IndexOf(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    const WVariantArray& a = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    WUInt32 uiStartIndex = inout_context.GetData<int>(node.GetInputDataOffset(2));

    WUInt32 uiIndex = a.IndexOf(element, uiStartIndex);
    inout_context.SetData<int>(node.GetOutputDataOffset(0), uiIndex == WInvalidIndex ? -1 : int(uiIndex));

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Insert(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(2));
    if (iIndex >= 0 && iIndex <= int(a.GetCount()))
    {
      a.InsertAt(iIndex, element);
      return ExecResult::RunNext(0);
    }

    WLog::Error("Visual script Array::Insert: Index '{}' is out of bounds. Valid range is [0, {}].", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  static ExecResult NodeFunction_Builtin_Array_PushBack(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    a.PushBack(element);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_PushBackRange(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariantArray& b = inout_context.GetData<WVariantArray>(node.GetInputDataOffset(1));
    a.PushBackRange(b);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Remove(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    const WVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    a.RemoveAndCopy(element);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_RemoveAt(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WVariantArray& a = inout_context.GetWritableData<WVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      a.RemoveAtAndCopy(iIndex);
      return ExecResult::RunNext(0);
    }

    WLog::Error("Visual script Array::RemoveAt: Index '{}' is out of bounds. Valid range is [0, {}).", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_CreateComponent(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    WTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != WGetStaticRTTI<WGameObject>())
    {
      WLog::Error("Visual script call CreateComponent: Game object is not of type 'WGameObject'");
      return ExecResult::Error();
    }

    if (p.m_pObject == nullptr)
    {
      WLog::Error("Visual script call CreateComponent: Game object is null");
      return ExecResult::Error();
    }

    WGameObject* pObject = static_cast<WGameObject*>(p.m_pObject);
    auto pComponentManager = pObject->GetWorld()->GetOrCreateManagerForComponentType(userData.m_pType);

    WComponent* pComponent = nullptr;
    pComponentManager->CreateComponent(pObject, pComponent);
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_TryGetComponentOfBaseType(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    WTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != WGetStaticRTTI<WGameObject>())
    {
      WLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'WGameObject'");
      return ExecResult::Error();
    }

    if (p.m_pObject == nullptr)
    {
      WLog::Error("Visual script call TryGetComponentOfBaseType: Game object is null");
      return ExecResult::Error();
    }

    WComponent* pComponent = nullptr;
    bool _ = static_cast<WGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_StartCoroutine(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto& userData = node.GetUserData<NodeUserData_StartCoroutine>();
    WString sName = inout_context.GetData<WString>(node.GetInputDataOffset(0));

    WScriptCoroutine* pCoroutine = nullptr;
    auto hCoroutine = pModule->CreateCoroutine(userData.m_pType, sName, inout_context.GetInstance(), userData.m_CreationMode, pCoroutine);
    pModule->StartCoroutine(hCoroutine, WArrayPtr<WVariant>());

    inout_context.SetData(node.GetOutputDataOffset(0), hCoroutine);


    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopCoroutine(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto hCoroutine = inout_context.GetData<WScriptCoroutineHandle>(node.GetInputDataOffset(0));
    if (pModule->IsCoroutineFinished(hCoroutine) == false)
    {
      pModule->StopAndDeleteCoroutine(hCoroutine);
    }
    else
    {
      auto& sName = inout_context.GetData<WString>(node.GetInputDataOffset(1));
      pModule->StopAndDeleteCoroutine(sName, &inout_context.GetInstance());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopAllCoroutines(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    pModule->StopAndDeleteAllCoroutines(&inout_context.GetInstance());

    return ExecResult::RunNext(0);
  }

  template <bool bWaitForAll>
  static ExecResult NodeFunction_Builtin_WaitForX(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    const WUInt32 uiNumCoroutines = node.m_NumInputDataOffsets;
    WUInt32 uiNumFinishedCoroutines = 0;

    for (WUInt32 i = 0; i < uiNumCoroutines; ++i)
    {
      auto hCoroutine = inout_context.GetData<WScriptCoroutineHandle>(node.GetInputDataOffset(i));
      if (pModule->IsCoroutineFinished(hCoroutine))
      {
        if constexpr (bWaitForAll == false)
        {
          return ExecResult::RunNext(0);
        }
        else
        {
          ++uiNumFinishedCoroutines;
        }
      }
    }

    if constexpr (bWaitForAll)
    {
      if (uiNumFinishedCoroutines == uiNumCoroutines)
      {
        return ExecResult::RunNext(0);
      }
    }

    return ExecResult::ContinueLater(WTime::MakeZero());
  }

  static ExecResult NodeFunction_Builtin_Yield(WVisualScriptExecutionContext& inout_context, const WVisualScriptGraphDescription::Node& node)
  {
    WScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      // set marker value of 0x1 to indicate we are in a yield
      inout_context.SetCurrentCoroutine(reinterpret_cast<WScriptCoroutine*>(0x1));

      return ExecResult::ContinueLater(WTime::MakeZero());
    }

    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  struct ExecuteFunctionContext
  {
    WVisualScriptGraphDescription::ExecuteFunction m_Func = nullptr;
    ExecuteFunctionGetter m_FuncGetter = nullptr;
  };

  static ExecuteFunctionContext s_TypeToExecuteFunctions[] = {
    {},                                                        // Invalid,
    {},                                                        // EntryCall,
    {},                                                        // EntryCall_Coroutine,
    {},                                                        // MessageHandler,
    {},                                                        // MessageHandler_Coroutine,
    {&NodeFunction_ReflectedFunction},                         // ReflectedFunction,
    {nullptr, &NodeFunction_GetReflectedProperty_Getter},      // GetReflectedProperty,
    {nullptr, &NodeFunction_SetReflectedProperty_Getter},      // SetReflectedProperty,
    {&NodeFunction_InplaceCoroutine},                          // InplaceCoroutine,
    {&NodeFunction_GetScriptOwner},                            // GetScriptOwner,
    {&NodeFunction_SendMessage},                               // SendMessage,

    {},                                                        // FirstBuiltin,

    {},                                                        // Builtin_Constant,
    {},                                                        // Builtin_GetVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter},       // Builtin_SetVariable,
    {nullptr, &NodeFunction_Builtin_IncVariable_Getter},       // Builtin_IncVariable,
    {nullptr, &NodeFunction_Builtin_DecVariable_Getter},       // Builtin_DecVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter},       // Builtin_TempVariable,

    {&NodeFunction_Builtin_Branch},                            // Builtin_Branch,
    {nullptr, &NodeFunction_Builtin_Switch_Getter},            // Builtin_Switch,
    {},                                                        // Builtin_WhileLoop,
    {},                                                        // Builtin_ForLoop,
    {},                                                        // Builtin_ForEachLoop,
    {},                                                        // Builtin_ReverseForEachLoop,
    {},                                                        // Builtin_Break,
    {},                                                        // Builtin_Jump,

    {&NodeFunction_Builtin_And},                               // Builtin_And,
    {&NodeFunction_Builtin_Or},                                // Builtin_Or,
    {&NodeFunction_Builtin_Not},                               // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter},           // Builtin_Compare,
    {},                                                        // Builtin_CompareExec,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter},           // Builtin_IsValid,
    {nullptr, &NodeFunction_Builtin_Select_Getter},            // Builtin_Select,

    {nullptr, &NodeFunction_Builtin_Add_Getter},               // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter},               // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter},               // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter},               // Builtin_Divide,
    {nullptr, &NodeFunction_Builtin_Mod_Getter},               // Builtin_Modulo,
    {nullptr, &NodeFunction_Builtin_Min_Getter},               // Builtin_Min,
    {nullptr, &NodeFunction_Builtin_Max_Getter},               // Builtin_Max,
    {nullptr, &NodeFunction_Builtin_Clamp_Getter},             // Builtin_Clamp,
    {&NodeFunction_Builtin_Expression},                        // Builtin_Expression,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {nullptr, &NodeFunction_Builtin_ToHashedString_Getter},    // Builtin_ToHashedString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_String_Format},                     // Builtin_String_Format,
    {&NodeFunction_Builtin_String_GetCharacterCount},          // Builtin_String_GetCharacterCount,
    {&NodeFunction_Builtin_String_IsEmpty},                    // Builtin_String_IsEmpty,

    {&NodeFunction_Builtin_MakeArray},                         // Builtin_MakeArray
    {&NodeFunction_Builtin_Array_GetElement},                  // Builtin_Array_GetElement,
    {&NodeFunction_Builtin_Array_SetElement},                  // Builtin_Array_SetElement,
    {&NodeFunction_Builtin_Array_GetCount},                    // Builtin_Array_GetCount,
    {&NodeFunction_Builtin_Array_IsEmpty},                     // Builtin_Array_IsEmpty,
    {&NodeFunction_Builtin_Array_Clear},                       // Builtin_Array_Clear,
    {&NodeFunction_Builtin_Array_Contains},                    // Builtin_Array_Contains,
    {&NodeFunction_Builtin_Array_IndexOf},                     // Builtin_Array_IndexOf,
    {&NodeFunction_Builtin_Array_Insert},                      // Builtin_Array_Insert,
    {&NodeFunction_Builtin_Array_PushBack},                    // Builtin_Array_PushBack,
    {&NodeFunction_Builtin_Array_PushBackRange},               // Builtin_Array_PushBackRange,
    {&NodeFunction_Builtin_Array_Remove},                      // Builtin_Array_Remove,
    {&NodeFunction_Builtin_Array_RemoveAt},                    // Builtin_Array_RemoveAt,

    {&NodeFunction_Builtin_CreateComponent},                   // Builtin_CreateComponent
    {&NodeFunction_Builtin_TryGetComponentOfBaseType},         // Builtin_TryGetComponentOfBaseType

    {&NodeFunction_Builtin_StartCoroutine},                    // Builtin_StartCoroutine,
    {&NodeFunction_Builtin_StopCoroutine},                     // Builtin_StopCoroutine,
    {&NodeFunction_Builtin_StopAllCoroutines},                 // Builtin_StopAllCoroutines,
    {&NodeFunction_Builtin_WaitForX<true>},                    // Builtin_WaitForAll,
    {&NodeFunction_Builtin_WaitForX<false>},                   // Builtin_WaitForAny,
    {&NodeFunction_Builtin_Yield},                             // Builtin_Yield,

    {},                                                        // LastBuiltin,
  };

  static_assert(W_ARRAY_SIZE(s_TypeToExecuteFunctions) == WVisualScriptNodeDescription::Type::Count);
} // namespace

WVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(WVisualScriptNodeDescription::Type::Enum nodeType, WVisualScriptDataType::Enum dataType)
{
  W_ASSERT_DEBUG(nodeType >= 0 && static_cast<WUInt32>(nodeType) < W_ARRAY_SIZE(s_TypeToExecuteFunctions), "Out of bounds access");
  auto& context = s_TypeToExecuteFunctions[nodeType];
  if (context.m_Func != nullptr)
  {
    return context.m_Func;
  }

  if (context.m_FuncGetter != nullptr)
  {
    return context.m_FuncGetter(dataType);
  }

  return nullptr;
}

#undef MAKE_EXEC_FUNC_GETTER
#undef MAKE_TONUMBER_EXEC_FUNC

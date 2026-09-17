#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

using SerializeFunction = WResult (*)(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_Size, WUInt32& out_alignment);
using DeserializeFunction = WResult (*)(WVisualScriptGraphDescription::Node& node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData);
using ToStringFunction = void (*)(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult);

namespace
{
  template <typename T, typename U>
  static WUInt32 GetDynamicSize(WUInt32 uiCount)
  {
    WUInt32 uiSize = sizeof(T);
    if (uiCount > 1)
    {
      uiSize += sizeof(U) * (uiCount - 1);
    }
    return uiSize;
  }


  template <typename T>
  static constexpr WUInt32 GetUserDataAlignment()
  {
    return WVisualScriptGraphDescription::Node::GetUserDataAlignment<T>();
  }

  struct NodeUserData_Type
  {
    const WRTTI* m_pType = nullptr;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding;
#endif

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      inout_stream << nodeDesc.m_sTargetTypeName;

      out_uiSize = sizeof(NodeUserData_Type);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Type>();
      return W_SUCCESS;
    }

    static WResult ReadType(WStreamReader& inout_stream, const WRTTI*& out_pType)
    {
      WStringBuilder sTypeName;
      inout_stream >> sTypeName;

      out_pType = WRTTI::FindTypeByName(sTypeName);
      if (out_pType == nullptr)
      {
        WLog::Error("Unknown type '{}'", sTypeName);
        return W_FAILURE;
      }

      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Type>(inout_pAdditionalData);
      W_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      if (nodeDesc.m_sTargetTypeName.IsEmpty() == false)
      {
        out_sResult.Append(nodeDesc.m_sTargetTypeName);
      }
    }
  };

  static_assert(sizeof(NodeUserData_Type) == 8);
  static_assert(GetUserDataAlignment<NodeUserData_Type>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperty : public NodeUserData_Type
  {
    const WAbstractProperty* m_pProperty = nullptr;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding;
#endif

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      W_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const WVariantArray& propertiesVar = nodeDesc.m_Value.Get<WVariantArray>();
      W_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      inout_stream << propertiesVar[0].Get<WHashedString>();

      out_uiSize = sizeof(NodeUserData_TypeAndProperty);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndProperty>();
      return W_SUCCESS;
    }

    template <typename T>
    static WResult ReadProperty(WStreamReader& inout_stream, const WRTTI* pType, WArrayPtr<T> properties, const WAbstractProperty*& out_pProp)
    {
      WStringBuilder sPropName;
      inout_stream >> sPropName;

      out_pProp = nullptr;
      for (auto& pProp : properties)
      {
        if (sPropName == pProp->GetPropertyName())
        {
          out_pProp = pProp;
          break;
        }
      }

      if (out_pProp == nullptr)
      {
        constexpr bool isFunction = std::is_same_v<T, const WAbstractFunctionProperty* const>;
        WLog::Error("{} '{}' not found on type '{}'", isFunction ? "Function" : "Property", sPropName, pType->GetTypeName());
        return W_FAILURE;
      }

      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperty>(inout_pAdditionalData);
      W_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      W_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_Value.IsA<WVariantArray>())
      {
        const WVariantArray& propertiesVar = nodeDesc.m_Value.Get<WVariantArray>();
        if (propertiesVar.IsEmpty() == false)
        {
          out_sResult.Append(".", propertiesVar[0].Get<WHashedString>());
        }
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndProperty>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperties : public NodeUserData_Type
  {
    WUInt32 m_uiNumProperties = 0;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding0;
#endif

    // This struct is allocated with enough space behind it to hold an array with m_uiNumProperties size.
    const WAbstractProperty* m_Properties[1];

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding1;
#endif

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      W_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const WVariantArray& propertiesVar = nodeDesc.m_Value.Get<WVariantArray>();

      WUInt32 uiCount = propertiesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : propertiesVar)
      {
        WHashedString sPropName = var.Get<WHashedString>();
        inout_stream << sPropName;
      }

      static_assert(sizeof(void*) <= sizeof(WUInt64));
      out_uiSize = GetDynamicSize<NodeUserData_TypeAndProperties, WUInt64>(uiCount);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndProperties>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      const WRTTI* pType = nullptr;
      W_SUCCEED_OR_RETURN(ReadType(inout_stream, pType));

      WUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const WUInt32 uiByteSize = GetDynamicSize<NodeUserData_TypeAndProperties, WUInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperties>(inout_pAdditionalData, uiByteSize);
      userData.m_pType = pType;
      userData.m_uiNumProperties = uiCount;

      WHybridArray<const WAbstractProperty*, 32> properties;
      userData.m_pType->GetAllProperties(properties);

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        const WAbstractProperty* pProperty = nullptr;
        W_SUCCEED_OR_RETURN(NodeUserData_TypeAndProperty::ReadProperty(inout_stream, userData.m_pType, properties.GetArrayPtr(), pProperty));
        userData.m_Properties[i] = pProperty;
      }

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperties) == 24);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndProperties>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndFunction : public NodeUserData_TypeAndProperty
  {
    WUInt32 m_uiInputArgsMask = 0;
    WUInt32 m_uiOutputArgsMask = 0;

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      W_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const WVariantArray& propertiesVar = nodeDesc.m_Value.Get<WVariantArray>();
      W_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      WHashedString sFunctionName = propertiesVar[0].Get<WHashedString>();
      inout_stream << sFunctionName;

      const WRTTI* pType = WRTTI::FindTypeByName(nodeDesc.m_sTargetTypeName);
      if (pType == nullptr)
        return W_FAILURE;

      const WAbstractFunctionProperty* pFunction = nullptr;
      for (auto pFunc : pType->GetFunctions())
      {
        if (pFunc->GetPropertyName() == sFunctionName)
        {
          pFunction = pFunc;
          break;
        }
      }

      if (pFunction == nullptr)
        return W_FAILURE;

      auto pScriptableFunctionAttribute = pFunction->GetAttributeByType<WScriptableFunctionAttribute>();
      if (pScriptableFunctionAttribute == nullptr)
        return W_FAILURE;

      WUInt32 uiInputArgsMask = 0;
      WUInt32 uiOutputArgsMask = 0;
      for (WUInt32 i = 0; i < pScriptableFunctionAttribute->GetArgumentCount(); ++i)
      {
        auto argType = pScriptableFunctionAttribute->GetArgumentType(i);
        if (argType == WScriptableFunctionAttribute::In || argType == WScriptableFunctionAttribute::Inout)
        {
          uiInputArgsMask |= W_BIT(i);
        }

        if (argType == WScriptableFunctionAttribute::Out || argType == WScriptableFunctionAttribute::Inout)
        {
          uiOutputArgsMask |= W_BIT(i);
        }
      }

      inout_stream << uiInputArgsMask;
      inout_stream << uiOutputArgsMask;

      out_uiSize = sizeof(NodeUserData_TypeAndFunction);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndFunction>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndFunction>(inout_pAdditionalData);
      W_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      W_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));

      inout_stream >> userData.m_uiInputArgsMask;
      inout_stream >> userData.m_uiOutputArgsMask;

      if (static_cast<const WAbstractFunctionProperty*>(userData.m_pProperty)->GetArgumentCount() != WMath::CountBits(userData.m_uiInputArgsMask | userData.m_uiOutputArgsMask))
      {
        WLog::Error("Visual script {} '{}': Argument count mismatch. Script needs re-transform.", WVisualScriptNodeDescription::Type::GetName(ref_node.m_Type), userData.m_pProperty->GetPropertyName());
        return W_FAILURE;
      }

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndFunction) == 24);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndFunction>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Switch
  {
    WUInt32 m_uiNumCases = 0;

    // This struct is allocated with enough space behind it to hold an array with m_uiNumCases size.
    WInt64 m_Cases[1];

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      const WVariantArray& casesVar = nodeDesc.m_Value.Get<WVariantArray>();

      WUInt32 uiCount = casesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : casesVar)
      {
        WInt64 iCaseValue = var.ConvertTo<WInt64>();
        inout_stream << iCaseValue;
      }

      out_uiSize = GetDynamicSize<NodeUserData_Switch, WInt64>(uiCount);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Switch>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      WUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const WUInt32 uiByteSize = GetDynamicSize<NodeUserData_Switch, WInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_Switch>(inout_pAdditionalData, uiByteSize);
      userData.m_uiNumCases = uiCount;

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        inout_stream >> userData.m_Cases[i];
      }

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  static_assert(sizeof(NodeUserData_Switch) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_Switch>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    WEnum<WComparisonOperator> m_ComparisonOperator;

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      WEnum<WComparisonOperator> compOp = static_cast<WComparisonOperator::Enum>(nodeDesc.m_Value.Get<WInt64>());
      inout_stream << compOp;

      out_uiSize = sizeof(NodeUserData_Comparison);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Comparison>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Comparison>(inout_pAdditionalData);
      inout_stream >> userData.m_ComparisonOperator;

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      WStringBuilder sCompOp;
      WReflectionUtils::EnumerationToString(WGetStaticRTTI<WComparisonOperator>(), nodeDesc.m_Value.Get<WInt64>(), sCompOp, WReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCompOp);
    }
  };

  static_assert(sizeof(NodeUserData_Comparison) == 1);
  static_assert(GetUserDataAlignment<NodeUserData_Comparison>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Expression
  {
    WExpressionByteCode m_ByteCode;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding[4];
#endif

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      const WExpressionByteCode& byteCode = nodeDesc.m_Value.Get<WExpressionByteCode>();

      WUInt32 uiDataSize = static_cast<WUInt32>(byteCode.GetDataBlob().GetCount());
      inout_stream << uiDataSize;

      W_SUCCEED_OR_RETURN(byteCode.Save(inout_stream));

      out_uiSize = sizeof(NodeUserData_Expression) + uiDataSize;
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Expression>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Expression>(inout_pAdditionalData);

      WUInt32 uiDataSize = 0;
      inout_stream >> uiDataSize;

      auto externalMemory = WMakeArrayPtr(inout_pAdditionalData, uiDataSize);
      inout_pAdditionalData += uiDataSize;

      W_SUCCEED_OR_RETURN(userData.m_ByteCode.Load(inout_stream, externalMemory));

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  static_assert(sizeof(NodeUserData_Expression) == 64);
  static_assert(GetUserDataAlignment<NodeUserData_Expression>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_StartCoroutine : public NodeUserData_Type
  {
    WEnum<WScriptCoroutineCreationMode> m_CreationMode;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding;
#endif

    static WResult Serialize(const WVisualScriptNodeDescription& nodeDesc, WStreamWriter& inout_stream, WUInt32& out_uiSize, WUInt32& out_uiAlignment)
    {
      W_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      WEnum<WScriptCoroutineCreationMode> creationMode = static_cast<WScriptCoroutineCreationMode::Enum>(nodeDesc.m_Value.Get<WInt64>());
      inout_stream << creationMode;

      out_uiSize = sizeof(NodeUserData_StartCoroutine);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_StartCoroutine>();
      return W_SUCCESS;
    }

    static WResult Deserialize(WVisualScriptGraphDescription::Node& ref_node, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_StartCoroutine>(inout_pAdditionalData);
      W_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      inout_stream >> userData.m_CreationMode;

      return W_SUCCESS;
    }

    static void ToString(const WVisualScriptNodeDescription& nodeDesc, WStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      WStringBuilder sCreationMode;
      WReflectionUtils::EnumerationToString(WGetStaticRTTI<WScriptCoroutineCreationMode>(), nodeDesc.m_Value.Get<WInt64>(), sCreationMode, WReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCreationMode);
    }
  };

  static_assert(sizeof(NodeUserData_StartCoroutine) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_StartCoroutine>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction m_SerializeFunc = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction m_ToStringFunc = nullptr;
  };

  inline UserDataContext s_TypeToUserDataContexts[] = {
    {},                                           // Invalid,
    {},                                           // EntryCall,
    {},                                           // EntryCall_Coroutine,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler_Coroutine,
    {&NodeUserData_TypeAndFunction::Serialize,
      &NodeUserData_TypeAndFunction::Deserialize,
      &NodeUserData_TypeAndFunction::ToString},   // ReflectedFunction,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize,
      &NodeUserData_TypeAndProperty::ToString},   // GetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize,
      &NodeUserData_TypeAndProperty::ToString},   // SetReflectedProperty,
    {&NodeUserData_TypeAndFunction::Serialize,
      &NodeUserData_TypeAndFunction::Deserialize,
      &NodeUserData_TypeAndFunction::ToString},   // InplaceCoroutine,
    {},                                           // GetScriptOwner,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // SendMessage,

    {},                                           // FirstBuiltin,

    {},                                           // Builtin_Constant,
    {},                                           // Builtin_GetVariable,
    {},                                           // Builtin_SetVariable,
    {},                                           // Builtin_IncVariable,
    {},                                           // Builtin_DecVariable,
    {},                                           // Builtin_TempVariable,

    {},                                           // Builtin_Branch,
    {&NodeUserData_Switch::Serialize,
      &NodeUserData_Switch::Deserialize,
      &NodeUserData_Switch::ToString},            // Builtin_Switch,
    {},                                           // Builtin_WhileLoop,
    {},                                           // Builtin_ForLoop,
    {},                                           // Builtin_ForEachLoop,
    {},                                           // Builtin_ReverseForEachLoop,
    {},                                           // Builtin_Break,
    {},                                           // Builtin_Jump,

    {},                                           // Builtin_And,
    {},                                           // Builtin_Or,
    {},                                           // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
      &NodeUserData_Comparison::Deserialize,
      &NodeUserData_Comparison::ToString},        // Builtin_Compare,
    {},                                           // Builtin_CompareExec,
    {},                                           // Builtin_IsValid,
    {},                                           // Builtin_Select,

    {},                                           // Builtin_Add,
    {},                                           // Builtin_Subtract,
    {},                                           // Builtin_Multiply,
    {},                                           // Builtin_Divide,
    {},                                           // Builtin_Modulo,
    {},                                           // Builtin_Min,
    {},                                           // Builtin_Max,
    {},                                           // Builtin_Clamp,
    {&NodeUserData_Expression::Serialize,
      &NodeUserData_Expression::Deserialize,
      &NodeUserData_Expression::ToString},        // Builtin_Expression,

    {},                                           // Builtin_ToBool,
    {},                                           // Builtin_ToByte,
    {},                                           // Builtin_ToInt,
    {},                                           // Builtin_ToInt64,
    {},                                           // Builtin_ToFloat,
    {},                                           // Builtin_ToDouble,
    {},                                           // Builtin_ToString,
    {},                                           // Builtin_ToHashedString,
    {},                                           // Builtin_ToVariant,
    {},                                           // Builtin_Variant_ConvertTo,

    {},                                           // Builtin_String_Format,
    {},                                           // Builtin_String_GetCharacterCount,
    {},                                           // Builtin_String_IsEmpty,

    {},                                           // Builtin_MakeArray
    {},                                           // Builtin_Array_GetElement,
    {},                                           // Builtin_Array_SetElement,
    {},                                           // Builtin_Array_GetCount,
    {},                                           // Builtin_Array_IsEmpty,
    {},                                           // Builtin_Array_Clear,
    {},                                           // Builtin_Array_Contains,
    {},                                           // Builtin_Array_IndexOf,
    {},                                           // Builtin_Array_Insert,
    {},                                           // Builtin_Array_PushBack,
    {},                                           // Builtin_Array_PushBackRange,
    {},                                           // Builtin_Array_Remove,
    {},                                           // Builtin_Array_RemoveAt,

    {&NodeUserData_Type::Serialize,
      &NodeUserData_Type::Deserialize,
      &NodeUserData_Type::ToString}, // Builtin_CreateComponent,
    {&NodeUserData_Type::Serialize,
      &NodeUserData_Type::Deserialize,
      &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType,

    {&NodeUserData_StartCoroutine::Serialize,
      &NodeUserData_StartCoroutine::Deserialize,
      &NodeUserData_StartCoroutine::ToString}, // Builtin_StartCoroutine,
    {},                                        // Builtin_StopCoroutine,
    {},                                        // Builtin_StopAllCoroutines,
    {},                                        // Builtin_WaitForAll,
    {},                                        // Builtin_WaitForAny,
    {},                                        // Builtin_Yield,

    {},                                        // LastBuiltin,
  };

  static_assert(W_ARRAY_SIZE(s_TypeToUserDataContexts) == WVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(WVisualScriptNodeDescription::Type::Enum nodeType)
{
  W_ASSERT_DEBUG(nodeType >= 0 && static_cast<WUInt32>(nodeType) < W_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}

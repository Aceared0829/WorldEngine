#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/World.h>
#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>

namespace
{
  constexpr const char* szPluginName = "EditorPluginVisualScript";
  static WHashedString sEventHandlerCategory = WMakeHashedString("Add Event Handler/");
  static WHashedString sCoroutinesCategory = WMakeHashedString("Coroutines");
  static WHashedString sPropertiesCategory = WMakeHashedString("Properties");
  static WHashedString sVariablesCategory = WMakeHashedString("Variables");
  static WHashedString sLogicCategory = WMakeHashedString("Logic");
  static WHashedString sMathCategory = WMakeHashedString("Math");
  static WHashedString sTypeConversionCategory = WMakeHashedString("Type Conversion");
  static WHashedString sStringCategory = WMakeHashedString("String");
  static WHashedString sArrayCategory = WMakeHashedString("Array");
  static WHashedString sMessagesCategory = WMakeHashedString("Messages");
  static WHashedString sEnumsCategory = WMakeHashedString("Enums");

  const WRTTI* FindTopMostBaseClass(const WRTTI* pRtti)
  {
    const WRTTI* pReflectedClass = WGetStaticRTTI<WReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  void CollectFunctionArgumentAttributes(const WAbstractFunctionProperty* pFuncProp, WDynamicArray<const WFunctionArgumentAttributes*>& out_attributes)
  {
    for (auto pAttr : pFuncProp->GetAttributes())
    {
      if (auto pFuncArgAttr = WDynamicCast<const WFunctionArgumentAttributes*>(pAttr))
      {
        WUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
        out_attributes.EnsureCount(uiArgIndex + 1);
        out_attributes[uiArgIndex] = pFuncArgAttr;
      }
    }
  }

  void AddInputProperty(WReflectedTypeDescriptor& ref_typeDesc, WStringView sName, const WRTTI* pRtti, WVisualScriptDataType::Enum scriptDataType, WArrayPtr<const WPropertyAttribute* const> attributes = {})
  {
    auto& propDesc = ref_typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = sName;
    propDesc.m_Flags = WPropertyFlags::StandardType;

    for (auto pAttr : attributes)
    {
      propDesc.m_Attributes.PushBack(pAttr->GetDynamicRTTI()->GetAllocator()->Clone<WPropertyAttribute>(pAttr));
    }

    if (pRtti->GetTypeFlags().IsSet(WTypeFlags::IsEnum))
    {
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sType = pRtti->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::IsEnum;
    }
    else if (pRtti->GetTypeFlags().IsSet(WTypeFlags::Bitflags))
    {
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sType = pRtti->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::Bitflags;
    }
    else
    {
      if (scriptDataType == WVisualScriptDataType::Color)
      {
        propDesc.m_Category = WPropertyCategory::Member;
        propDesc.m_sType = pRtti->GetTypeName();
        propDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WExposeColorAlphaAttribute));
      }
      else if (scriptDataType == WVisualScriptDataType::Variant)
      {
        propDesc.m_Category = WPropertyCategory::Member;
        propDesc.m_sType = WGetStaticRTTI<WVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WVisualScriptVariableAttribute));
      }
      else if (scriptDataType == WVisualScriptDataType::Array)
      {
        propDesc.m_Category = WPropertyCategory::Array;
        propDesc.m_sType = WGetStaticRTTI<WVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WVisualScriptVariableAttribute));
      }
      else if (scriptDataType == WVisualScriptDataType::Map)
      {
        propDesc.m_Category = WPropertyCategory::Map;
        propDesc.m_sType = WGetStaticRTTI<WVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WVisualScriptVariableAttribute));
      }
      else
      {
        propDesc.m_Category = WPropertyCategory::Member;
        propDesc.m_sType = WVisualScriptDataType::GetRtti(scriptDataType)->GetTypeName();
      }
    }
  }

  WStringView StripTypeName(WStringView sTypeName)
  {
    sTypeName.TrimWordStart("W");
    return sTypeName;
  }

  WStringView GetTypeName(const WRTTI* pRtti)
  {
    WStringView sTypeName = pRtti->GetTypeName();
    if (auto pScriptExtension = pRtti->GetAttributeByType<WScriptExtensionAttribute>())
    {
      sTypeName = pScriptExtension->GetTypeName();
    }
    return StripTypeName(sTypeName);
  }

  WColorGammaUB NiceColorFromName(WStringView sTypeName, WStringView sCategory = WStringView())
  {
    float typeX = WSimdRandom::FloatZeroToOne(WSimdVec4i(WHashingUtils::StringHash(sTypeName))).x();

    float x = typeX;
    if (sCategory.IsEmpty() == false)
    {
      x = WSimdRandom::FloatZeroToOne(WSimdVec4i(WHashingUtils::StringHash(sCategory))).x();
      x += typeX * WColorScheme::s_fIndexNormalizer;
    }

    return WColorScheme::DarkUI(x);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

static WColorScheme::Enum s_scriptDataTypeToPinColor[] = {
  WColorScheme::Gray,   // Invalid
  WColorScheme::Red,    // Bool,
  WColorScheme::Cyan,   // Byte,
  WColorScheme::Teal,   // Int,
  WColorScheme::Teal,   // Int64,
  WColorScheme::Green,  // Float,
  WColorScheme::Green,  // Double,
  WColorScheme::Lime,   // Color,
  WColorScheme::Orange, // Vector2,
  WColorScheme::Orange, // Vector3,
  WColorScheme::Orange, // Vector4,
  WColorScheme::Orange, // Quaternion,
  WColorScheme::Orange, // Transform,
  WColorScheme::Violet, // Time,
  WColorScheme::Green,  // Angle,
  WColorScheme::Grape,  // String,
  WColorScheme::Grape,  // HashedString,
  WColorScheme::Blue,   // GameObject,
  WColorScheme::Blue,   // Component,
  WColorScheme::Blue,   // TypedPointer,
  WColorScheme::Pink,   // Variant,
  WColorScheme::Pink,   // VariantArray,
  WColorScheme::Pink,   // VariantDictionary,
  WColorScheme::Cyan,   // Coroutine,
};

static_assert(W_ARRAY_SIZE(s_scriptDataTypeToPinColor) == WVisualScriptDataType::Count);

// static
WColor WVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Enum dataType)
{
  if (dataType == WVisualScriptDataType::EnumValue || dataType == WVisualScriptDataType::BitflagValue)
  {
    return WColorScheme::DarkUI(WColorScheme::Teal);
  }

  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_scriptDataTypeToPinColor), "Out of bounds access");
  return WColorScheme::DarkUI(s_scriptDataTypeToPinColor[dataType]);
}

WColor WVisualScriptNodeRegistry::PinDesc::GetColor() const
{
  if (IsExecutionPin())
  {
    return WColorScheme::DarkUI(WColorScheme::Gray);
  }

  if (m_ScriptDataType > WVisualScriptDataType::Invalid && m_ScriptDataType < WVisualScriptDataType::Count)
  {
    return GetColorForScriptDataType(m_ScriptDataType);
  }

  if (m_ScriptDataType == WVisualScriptDataType::EnumValue || m_ScriptDataType == WVisualScriptDataType::BitflagValue)
  {
    return WColorScheme::DarkUI(WColorScheme::Teal);
  }

  if (m_ScriptDataType == WVisualScriptDataType::Any)
  {
    return WColorScheme::DarkUI(WColorScheme::Gray);
  }

  return WColorScheme::DarkUI(WColorScheme::Blue);
}

//////////////////////////////////////////////////////////////////////////

void AddExecutionPin(WVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, WStringView sName, WHashedString sDynamicPinProperty, bool bSplitExecution, WSmallArray<WVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = nullptr;
  pin.m_ScriptDataType = WVisualScriptDataType::Invalid;
  pin.m_bSplitExecution = bSplitExecution;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void WVisualScriptNodeRegistry::NodeDesc::AddInputExecutionPin(WStringView sName, const WHashedString& sDynamicPinProperty /*= WHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, false, m_InputPins);

  m_bImplicitExecution = false;
}

void WVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(WStringView sName, const WHashedString& sDynamicPinProperty /*= WHashedString()*/, bool bSplitExecution /*= false*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, bSplitExecution, m_OutputPins);

  m_bImplicitExecution = false;
}

void AddDataPin(WVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, WStringView sName, const WRTTI* pDataType, WVisualScriptDataType::Enum scriptDataType, bool bRequired, WHashedString sDynamicPinProperty, WVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, bool bReplaceWithArray, WSmallArray<WVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  if ((scriptDataType == WVisualScriptDataType::AnyPointer || scriptDataType == WVisualScriptDataType::Any) && deductTypeFunc == nullptr)
  {
    deductTypeFunc = &WVisualScriptTypeDeduction::DeductFromNodeDataType;
  }

  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_DeductTypeFunc = deductTypeFunc;
  pin.m_pDataType = pDataType;
  pin.m_ScriptDataType = scriptDataType;
  pin.m_bRequired = bRequired;
  pin.m_bReplaceWithArray = bReplaceWithArray;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void WVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(WStringView sName, const WRTTI* pDataType, WVisualScriptDataType::Enum scriptDataType, bool bRequired, const WHashedString& sDynamicPinProperty /*= WHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/, bool bReplaceWithArray /*= false*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, bReplaceWithArray, m_InputPins);
}

void WVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(WStringView sName, const WRTTI* pDataType, WVisualScriptDataType::Enum scriptDataType, const WHashedString& sDynamicPinProperty /*= WHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, false, m_OutputPins);
}

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_SINGLETON(WVisualScriptNodeRegistry);

WVisualScriptNodeRegistry::WVisualScriptNodeRegistry()
  : m_SingletonRegistrar(this)
{
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));

  UpdateNodeTypes();
}

WVisualScriptNodeRegistry::~WVisualScriptNodeRegistry()
{
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));
}

void WVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e)
{
  if (e.m_pChangedType->GetPluginName() == "EditorPluginVisualScript")
    return;

  if ((e.m_Type == WPhantomRttiManagerEvent::Type::TypeAdded && m_TypeToNodeDescs.Contains(e.m_pChangedType) == false) ||
      e.m_Type == WPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);

    // Also update dependent types
    for (const WRTTI* pRtti : m_TypesToUpdate)
    {
      if (m_ExposedTypes.Contains(pRtti) == false)
        UpdateNodeType(pRtti, true);
    }
    m_TypesToUpdate.Clear();
  }
}

void WVisualScriptNodeRegistry::UpdateNodeTypes()
{
  W_PROFILE_SCOPE("Update VS Node Types");

  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    WReflectedTypeDescriptor desc;
    desc.m_sTypeName = "WVisualScriptNodeBase";
    desc.m_sPluginName = szPluginName;
    desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
    desc.m_Flags = WTypeFlags::Abstract | WTypeFlags::Class;

    m_pBaseType = WPhantomRttiManager::RegisterType(desc);
  }

  if (m_bBuiltinTypesCreated == false)
  {
    CreateBuiltinTypes();
    m_bBuiltinTypesCreated = true;
  }

  auto& scriptBaseClassesDynEnum = WDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  WRTTI::ForEachType([this](const WRTTI* pRtti)
    { UpdateNodeType(pRtti); });

  // Also update dependent types
  for (const WRTTI* pRtti : m_TypesToUpdate)
  {
    if (m_ExposedTypes.Contains(pRtti) == false)
      UpdateNodeType(pRtti, true);
  }
  m_TypesToUpdate.Clear();
}

void WVisualScriptNodeRegistry::UpdateNodeType(const WRTTI* pRtti, bool bForceExpose /*= false*/)
{
  static WHashedString sType = WMakeHashedString("Type");
  static WHashedString sProperty = WMakeHashedString("Property");
  static WHashedString sValue = WMakeHashedString("Value");

  if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<WExcludeFromScript>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<WScriptCoroutine>())
  {
    CreateCoroutineNodeType(pRtti);
  }
  else if (pRtti->IsDerivedFrom<WMessage>())
  {
    CreateMessageNodeTypes(pRtti);
  }
  else
  {
    // expose reflected functions and properties to visual scripts
    {
      // All components should be exposed to visual scripts, furthermore all classes that have script-able functions are also exposed
      bool bExposeToVisualScript = pRtti->IsDerivedFrom<WComponent>() || bForceExpose;
      bool bHasBaseClassFunctions = false;

      WStringBuilder sCategory;
      {
        WStringView sTypeName = GetTypeName(pRtti);
        const WRTTI* pBaseClass = FindTopMostBaseClass(pRtti);
        if (pBaseClass != pRtti)
        {
          sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
        }
        else
        {
          sCategory = sTypeName;
        }
      }

      WHashedString sCategoryHashed;
      sCategoryHashed.Assign(sCategory);

      for (const WAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
      {
        auto pScriptableFunctionAttribute = pFuncProp->GetAttributeByType<WScriptableFunctionAttribute>();
        if (pScriptableFunctionAttribute == nullptr)
          continue;

        bExposeToVisualScript = true;

        bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<WScriptBaseClassFunctionAttribute>() != nullptr;
        if (bIsBaseClassFunction)
        {
          bHasBaseClassFunctions = true;
        }

        CreateFunctionCallNodeType(pRtti, bIsBaseClassFunction ? sEventHandlerCategory : sCategoryHashed, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript && m_ExposedTypes.Insert(pRtti) == false)
      {
        WStringView sTypeName = GetTypeName(pRtti);
        WStringBuilder sPropertyNodeTypeName;

        for (const WAbstractProperty* pProp : pRtti->GetProperties())
        {
          if (pProp->GetCategory() != WPropertyCategory::Member)
            continue;

          const WRTTI* pPropRtti = pProp->GetSpecificType();
          if (pPropRtti->GetTypeFlags().IsSet(WTypeFlags::IsEnum))
          {
            CreateEnumNodeTypes(pPropRtti);
          }

          WUInt32 uiStart = m_PropertyValues.GetCount();
          m_PropertyValues.PushBack({sType, sTypeName});
          m_PropertyValues.PushBack({sProperty, pProp->GetPropertyName()});

          // String views are not allowed in command history, so we need to convert the value into a proper string.
          WVariant defaultValue = WReflectionUtils::GetDefaultValue(pProp);
          if (defaultValue.IsA<WStringView>())
          {
            defaultValue = defaultValue.ConvertTo<WString>();
          }
          m_PropertyValues.PushBack({sValue, defaultValue});

          // Setter
          {
            sPropertyNodeTypeName.Set("Set", pProp->GetPropertyName());
            auto it = m_PropertyNodeTypeNames.Insert(sPropertyNodeTypeName);

            auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType = m_pSetPropertyType;
            nodeTemplate.m_sTypeName = it.Key();
            nodeTemplate.m_sCategory = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 3;
          }

          // Getter
          {
            sPropertyNodeTypeName.Set("Get", pProp->GetPropertyName());
            auto it = m_PropertyNodeTypeNames.Insert(sPropertyNodeTypeName);

            auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType = m_pGetPropertyType;
            nodeTemplate.m_sTypeName = it.Key();
            nodeTemplate.m_sCategory = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 2;
          }
        }
      }

      if (bHasBaseClassFunctions)
      {
        auto& scriptBaseClassesDynEnum = WDynamicStringEnum::GetDynamicEnum("ScriptBaseClasses");
        scriptBaseClassesDynEnum.AddValidValue(StripTypeName(pRtti->GetTypeName()));

        CreateGetOwnerNodeType(pRtti);
      }
    }
  }
}

WResult WVisualScriptNodeRegistry::GetScriptDataType(const WRTTI* pRtti, WVisualScriptDataType::Enum& out_scriptDataType, WStringView sFunctionName /*= WStringView()*/, WStringView sArgName /*= WStringView()*/)
{
  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::IsEnum))
  {
    CreateEnumNodeTypes(pRtti);
  }

  WVisualScriptDataType::Enum scriptDataType = WVisualScriptDataType::FromRtti(pRtti);
  if (scriptDataType == WVisualScriptDataType::Invalid)
  {
    WLog::Warning("The script function '{}' uses an argument '{}' of type '{}' which is not a valid script data type, therefore this function will not be available in visual scripts", sFunctionName, sArgName, pRtti->GetTypeName());
    return W_FAILURE;
  }

  out_scriptDataType = scriptDataType;
  return W_SUCCESS;
}

WVisualScriptDataType::Enum WVisualScriptNodeRegistry::GetScriptDataType(const WAbstractProperty* pProp)
{
  if (pProp->GetCategory() == WPropertyCategory::Member)
  {
    WVisualScriptDataType::Enum result = WVisualScriptDataType::Invalid;
    GetScriptDataType(pProp->GetSpecificType(), result, "Member", pProp->GetPropertyName()).IgnoreResult();
    return result;
  }
  else if (pProp->GetCategory() == WPropertyCategory::Array)
  {
    return WVisualScriptDataType::Array;
  }
  else if (pProp->GetCategory() == WPropertyCategory::Map)
  {
    return WVisualScriptDataType::Map;
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return WVisualScriptDataType::Invalid;
}

template <typename T>
void WVisualScriptNodeRegistry::AddInputDataPin(WReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, WStringView sName)
{
  const WRTTI* pDataType = WGetStaticRTTI<T>();

  WVisualScriptDataType::Enum scriptDataType;
  W_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  AddInputProperty(ref_typeDesc, sName, pDataType, scriptDataType);

  ref_nodeDesc.AddInputDataPin(sName, pDataType, scriptDataType, false);
};

void WVisualScriptNodeRegistry::AddInputDataPin_Any(WReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, WStringView sName, bool bRequired, bool bAddVariantProperty /*= false*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  if (bAddVariantProperty)
  {
    AddInputProperty(ref_typeDesc, sName, WGetStaticRTTI<WVariant>(), WVisualScriptDataType::Variant);
  }

  ref_nodeDesc.AddInputDataPin(sName, nullptr, WVisualScriptDataType::Any, bRequired, WHashedString(), deductTypeFunc);
}

template <typename T>
void WVisualScriptNodeRegistry::AddOutputDataPin(NodeDesc& ref_nodeDesc, WStringView sName)
{
  const WRTTI* pDataType = WGetStaticRTTI<T>();

  WVisualScriptDataType::Enum scriptDataType;
  W_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  ref_nodeDesc.AddOutputDataPin(sName, pDataType, scriptDataType);
};

void WVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  const WColorGammaUB logicColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Invalid);
  const WColorGammaUB mathColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Int);
  const WColorGammaUB stringColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::String);
  const WColorGammaUB gameObjectColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::GameObject);
  const WColorGammaUB variantColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Variant);
  const WColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Coroutine);

  WReflectedTypeDescriptor typeDesc;

  // GetReflectedProperty
  {
    FillDesc(typeDesc, "GetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, WVisualScriptDataType::Any, true, WHashedString(), &WVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, WVisualScriptDataType::Any);

    m_pGetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // SetReflectedProperty
  {
    FillDesc(typeDesc, "SetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{Type}::Set {Property} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::SetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Object", nullptr, WVisualScriptDataType::Any, true, WHashedString(), &WVisualScriptTypeDeduction::DeductFromTypeProperty);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_pSetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // Builtin_GetVariable
  {
    FillDesc(typeDesc, "Builtin_GetVariable", logicColor);

    AddInputProperty(typeDesc, "Name", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, WVisualScriptDataType::Any);

    m_pGetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_SetVariable
  {
    FillDesc(typeDesc, "Builtin_SetVariable", logicColor);

    AddInputProperty(typeDesc, "Name", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Set {Name} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_SetVariable;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, WVisualScriptDataType::Any);

    m_pSetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_IncVariable, Builtin_DecVariable
  {
    WVisualScriptNodeDescription::Type::Enum nodeTypes[] = {
      WVisualScriptNodeDescription::Type::Builtin_IncVariable,
      WVisualScriptNodeDescription::Type::Builtin_DecVariable,
    };

    const char* szNodeTitles[] = {
      "++ {Name}",
      "-- {Name}",
    };

    static_assert(W_ARRAY_SIZE(nodeTypes) == W_ARRAY_SIZE(szNodeTitles));

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(nodeTypes); ++i)
    {
      FillDesc(typeDesc, WVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), logicColor);

      AddInputProperty(typeDesc, "Name", WGetStaticRTTI<WString>(), WVisualScriptDataType::String);

      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, WVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
    }
  }

  // Builtin_TempVariable
  {
    FillDesc(typeDesc, "Builtin_TempVariable", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_TempVariable;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, WVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_Branch
  {
    FillDesc(typeDesc, "Builtin_Branch", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Switch
  {
    WVisualScriptDataType::Enum switchDataTypes[] = {
      WVisualScriptDataType::Int64,
      WVisualScriptDataType::HashedString,
    };

    const char* szSwitchTypeNames[] = {
      "Builtin_SwitchInt64",
      "Builtin_SwitchString",
    };

    const char* szSwitchTitles[] = {
      "Int64::Switch",
      "HashedString::Switch",
    };

    static_assert(W_ARRAY_SIZE(switchDataTypes) == W_ARRAY_SIZE(szSwitchTypeNames));
    static_assert(W_ARRAY_SIZE(switchDataTypes) == W_ARRAY_SIZE(szSwitchTitles));

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(switchDataTypes); ++i)
    {
      const WRTTI* pValueType = WVisualScriptDataType::GetRtti(switchDataTypes[i]);

      FillDesc(typeDesc, szSwitchTypeNames[i], logicColor);

      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = WPropertyCategory::Array;
        propDesc.m_sName = "Cases";
        propDesc.m_sType = pValueType->GetTypeName();
        propDesc.m_Flags = WPropertyFlags::StandardType;

        auto pMaxSizeAttr = W_DEFAULT_NEW(WMaxArraySizeAttribute, 16);
        propDesc.m_Attributes.PushBack(pMaxSizeAttr);

        auto pNoTempAttr = W_DEFAULT_NEW(WNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);
      }

      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, szSwitchTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Switch;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("Case", WMakeHashedString("Cases"));
      nodeDesc.AddOutputExecutionPin("Default");

      nodeDesc.AddInputDataPin("Value", pValueType, switchDataTypes[i], true);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
    }
  }

  // Builtin_WhileLoop
  {
    FillDesc(typeDesc, "Builtin_WhileLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForLoop
  {
    FillDesc(typeDesc, "Builtin_ForLoop", logicColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "ForLoop [{FirstIndex}..{LastIndex}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_ForLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<int>(typeDesc, nodeDesc, "FirstIndex");
    AddInputDataPin<int>(typeDesc, nodeDesc, "LastIndex");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_ForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<WVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<WVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ReverseForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ReverseForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<WVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<WVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Break
  {
    FillDesc(typeDesc, "Builtin_Break", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_And
  {
    FillDesc(typeDesc, "Builtin_And", logicColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Or
  {
    FillDesc(typeDesc, "Builtin_Or", logicColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Not
  {
    FillDesc(typeDesc, "Builtin_Not", logicColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Compare
  {
    FillDesc(typeDesc, "Builtin_Compare", logicColor);

    AddInputProperty(typeDesc, "Operator", WGetStaticRTTI<WComparisonOperator>(), WVisualScriptDataType::Int64);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_CompareExec
  {
    FillDesc(typeDesc, "Builtin_CompareExec", logicColor);

    AddInputProperty(typeDesc, "Operator", WGetStaticRTTI<WComparisonOperator>(), WVisualScriptDataType::Int64);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_CompareExec;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_IsValid
  {
    FillDesc(typeDesc, "Builtin_IsValid", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Select
  {
    FillDesc(typeDesc, "Builtin_Select", logicColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "{Condition} ? {A} : {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Select;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    nodeDesc.AddOutputDataPin("", nullptr, WVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Add, Builtin_Sub, Builtin_Mul, Builtin_Div, Builtin_Modulo, Builtin_Min, Builtin_Max
  {
    WVisualScriptNodeDescription::Type::Enum mathNodeTypes[] = {
      WVisualScriptNodeDescription::Type::Builtin_Add,
      WVisualScriptNodeDescription::Type::Builtin_Subtract,
      WVisualScriptNodeDescription::Type::Builtin_Multiply,
      WVisualScriptNodeDescription::Type::Builtin_Divide,
      WVisualScriptNodeDescription::Type::Builtin_Modulo,
      WVisualScriptNodeDescription::Type::Builtin_Min,
      WVisualScriptNodeDescription::Type::Builtin_Max,
    };

    const char* szMathNodeTitles[] = {
      "{A} + {B}",
      "{A} - {B}",
      "{A} * {B}",
      "{A} / {B}",
      "{A} % {B}",
      "Min({A}, {B})",
      "Max({A}, {B})",
    };

    static_assert(W_ARRAY_SIZE(mathNodeTypes) == W_ARRAY_SIZE(szMathNodeTitles));

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(mathNodeTypes); ++i)
    {
      FillDesc(typeDesc, WVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), mathColor);

      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, WVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
    }
  }

  // Builtin_Clamp
  {
    FillDesc(typeDesc, "Builtin_Clamp", mathColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Clamp({X}, {Min}, {Max})");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Clamp;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");
    AddInputDataPin_Any(typeDesc, nodeDesc, "X", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Min", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Max", false, true);
    nodeDesc.AddOutputDataPin("", nullptr, WVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
  }

  // Builtin_Expression
  {
    FillDesc(typeDesc, "Builtin_Expression", mathColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sName = "Expression";
      propDesc.m_sType = WGetStaticRTTI<WString>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::StandardType;

      auto pExpressionWidgetAttr = W_DEFAULT_NEW(WExpressionWidgetAttribute, "Inputs", "Outputs");
      propDesc.m_Attributes.PushBack(pExpressionWidgetAttr);
    }

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Array;
      propDesc.m_sName = "Inputs";
      propDesc.m_sType = WGetStaticRTTI<WVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::Class;

      auto pMaxSizeAttr = W_DEFAULT_NEW(WMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Array;
      propDesc.m_sName = "Outputs";
      propDesc.m_sType = WGetStaticRTTI<WVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::Class;

      auto pMaxSizeAttr = W_DEFAULT_NEW(WMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Expression::{Expression}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Expression;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductDummy;

    nodeDesc.AddInputDataPin("Input", nullptr, WVisualScriptDataType::Any, false, WMakeHashedString("Inputs"), &WVisualScriptTypeDeduction::DeductFromExpressionInput);
    nodeDesc.AddOutputDataPin("Output", nullptr, WVisualScriptDataType::Any, WMakeHashedString("Outputs"), &WVisualScriptTypeDeduction::DeductFromExpressionOutput);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      WColorGammaUB m_Color;
      WVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {logicColor, WVisualScriptDataType::Bool},
      {mathColor, WVisualScriptDataType::Byte},
      {mathColor, WVisualScriptDataType::Int},
      {mathColor, WVisualScriptDataType::Int64},
      {mathColor, WVisualScriptDataType::Float},
      {mathColor, WVisualScriptDataType::Double},
      {stringColor, WVisualScriptDataType::String},
      {variantColor, WVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = WVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      FillDesc(typeDesc, WVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", WVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
    }
  }

  // Builtin_Variant_ConvertTo
  {
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", variantColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sName = "Type";
      propDesc.m_sType = WGetStaticRTTI<WVisualScriptDataType>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::IsEnum;

      auto pAttr = W_DEFAULT_NEW(WDefaultValueAttribute, WVisualScriptDataType::Bool);
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Variant::ConvertTo {Type}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Variant_ConvertTo;
    nodeDesc.m_DeductTypeFunc = &WVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("Succeeded");
    nodeDesc.AddOutputExecutionPin("Failed");
    nodeDesc.AddInputDataPin("Variant", WGetStaticRTTI<WVariant>(), WVisualScriptDataType::Variant, true);
    nodeDesc.AddOutputDataPin("Result", nullptr, WVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
  }

  // Builtin_String_Format
  {
    FillDesc(typeDesc, "Builtin_String_Format", stringColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<WString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", WGetStaticRTTI<WVariant>(), WVisualScriptDataType::Variant, false, WMakeHashedString("Params"));
    AddOutputDataPin<WString>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sStringCategory);
  }

  // Builtin_String_GetCharacterCount
  {
    FillDesc(typeDesc, "Builtin_String::GetCharacterCount", stringColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_String_GetCharacterCount;

    AddInputDataPin<WString>(typeDesc, nodeDesc, "Text");
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sStringCategory);
  }

  // Builtin_String_IsEmpty
  {
    FillDesc(typeDesc, "Builtin_String::IsEmpty", stringColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_String_IsEmpty;

    AddInputDataPin<WString>(typeDesc, nodeDesc, "Text");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sStringCategory);
  }

  // Builtin_MakeArray
  {
    FillDesc(typeDesc, "Builtin_MakeArray", variantColor);

    WHashedString sElements = WMakeHashedString("Elements");
    AddInputProperty(typeDesc, sElements, WGetStaticRTTI<WVariant>(), WVisualScriptDataType::Array);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin(sElements, WGetStaticRTTI<WVariant>(), WVisualScriptDataType::Variant, false, sElements);
    nodeDesc.AddOutputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetElement
  {
    FillDesc(typeDesc, "Builtin_Array_GetElement", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::GetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_GetElement;

    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddOutputDataPin<WVariant>(nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_SetElement
  {
    FillDesc(typeDesc, "Builtin_Array_SetElement", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::SetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_SetElement;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetCount
  {
    FillDesc(typeDesc, "Builtin_Array::GetCount", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_GetCount;

    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IsEmpty
  {
    FillDesc(typeDesc, "Builtin_Array::IsEmpty", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_IsEmpty;

    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Clear
  {
    FillDesc(typeDesc, "Builtin_Array::Clear", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_Clear;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Contains
  {
    FillDesc(typeDesc, "Builtin_Array_Contains", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::Contains {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_Contains;

    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IndexOf
  {
    FillDesc(typeDesc, "Builtin_Array_IndexOf", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::IndexOf {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_IndexOf;

    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "StartIndex");
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Insert
  {
    FillDesc(typeDesc, "Builtin_Array::Insert", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_Insert;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_PushBack
  {
    FillDesc(typeDesc, "Builtin_Array::PushBack", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_PushBack;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_PushBackRange
  {
    FillDesc(typeDesc, "Builtin_Array::PushBackRange", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_PushBackRange;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    nodeDesc.AddInputDataPin("Range", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Remove
  {
    FillDesc(typeDesc, "Builtin_Array_Remove", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::Remove {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_Remove;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<WVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_RemoveAt
  {
    FillDesc(typeDesc, "Builtin_Array_RemoveAt", variantColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "Array::RemoveAt {Index}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Array_RemoveAt;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", WGetStaticRTTI<WVariantArray>(), WVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_CreateComponent
  {
    FillDesc(typeDesc, "Builtin_CreateComponent", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = WGetStaticRTTI<WString>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::StandardType;

      auto pAttr = W_DEFAULT_NEW(WRttiTypeStringAttribute, "WComponent");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "GameObject::Create {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_CreateComponent;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("GameObject", WGetStaticRTTI<WGameObject>(), WVisualScriptDataType::GameObject, false);
    AddOutputDataPin<WComponent>(nodeDesc, "Component");

    RegisterNodeType(typeDesc, std::move(nodeDesc), WMakeHashedString("GameObject"));
  }

  // Builtin_TryGetComponentOfBaseType
  {
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = WPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = WGetStaticRTTI<WString>()->GetTypeName();
      propDesc.m_Flags = WPropertyFlags::StandardType;

      auto pAttr = W_DEFAULT_NEW(WRttiTypeStringAttribute, "WComponent");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "GameObject::TryGet {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", WGetStaticRTTI<WGameObject>(), WVisualScriptDataType::GameObject, false);
    AddOutputDataPin<WComponent>(nodeDesc, "Component");

    RegisterNodeType(typeDesc, std::move(nodeDesc), WMakeHashedString("GameObject"));
  }

  // Builtin_StartCoroutine
  {
    FillDesc(typeDesc, "Builtin_StartCoroutine", coroutineColor);

    AddInputProperty(typeDesc, "CoroutineMode", WGetStaticRTTI<WScriptCoroutineCreationMode>(), WVisualScriptDataType::Int64);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "StartCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_StartCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("CoroutineBody", WHashedString(), true);
    AddInputDataPin<WString>(typeDesc, nodeDesc, "Name");
    nodeDesc.AddOutputDataPin("CoroutineID", WGetStaticRTTI<WScriptCoroutineHandle>(), WVisualScriptDataType::Coroutine);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopCoroutine
  {
    FillDesc(typeDesc, "Builtin_StopCoroutine", coroutineColor);

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", WGetStaticRTTI<WScriptCoroutineHandle>(), WVisualScriptDataType::Coroutine, false);
    AddInputDataPin<WString>(typeDesc, nodeDesc, "Name");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopAllCoroutines
  {
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_WaitForAll
  {
    WVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      WVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      WVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      FillDesc(typeDesc, WVisualScriptNodeDescription::Type::GetName(waitType), coroutineColor);

      WHashedString sCount = WMakeHashedString("Count");
      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = WPropertyCategory::Member;
        propDesc.m_sName = sCount.GetView();
        propDesc.m_sType = WGetStaticRTTI<WUInt32>()->GetTypeName();
        propDesc.m_Flags = WPropertyFlags::StandardType;

        auto pNoTempAttr = W_DEFAULT_NEW(WNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);

        auto pDefaultAttr = W_DEFAULT_NEW(WDefaultValueAttribute, 1);
        propDesc.m_Attributes.PushBack(pDefaultAttr);

        auto pClampAttr = W_DEFAULT_NEW(WClampValueAttribute, 1, 16);
        propDesc.m_Attributes.PushBack(pClampAttr);
      }

      NodeDesc nodeDesc;
      nodeDesc.m_Type = waitType;

      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddInputDataPin("", WGetStaticRTTI<WScriptCoroutineHandle>(), WVisualScriptDataType::Coroutine, false, sCount);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
    }
  }

  // Builtin_Yield
  {
    FillDesc(typeDesc, "Builtin_Yield", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }
}

void WVisualScriptNodeRegistry::CreateGetOwnerNodeType(const WRTTI* pRtti)
{
  WStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  WReflectedTypeDescriptor typeDesc;
  {
    WStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    WColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, color);
  }

  NodeDesc nodeDesc;
  nodeDesc.m_sFilterByBaseClass.Assign(sBaseClass);
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_Type = WVisualScriptNodeDescription::Type::GetScriptOwner;

  WVisualScriptDataType::Enum scriptDataType;
  if (GetScriptDataType(pRtti, scriptDataType, "GetScriptOwner", "").Failed())
    return;

  if (pRtti->IsDerivedFrom<WComponent>())
  {
    nodeDesc.AddOutputDataPin("World", WGetStaticRTTI<WWorld>(), WVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("GameObject", WGetStaticRTTI<WGameObject>(), WVisualScriptDataType::GameObject);
    nodeDesc.AddOutputDataPin("Component", WGetStaticRTTI<WComponent>(), WVisualScriptDataType::Component);
  }
  else
  {
    nodeDesc.AddOutputDataPin("World", WGetStaticRTTI<WWorld>(), WVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("Owner", pRtti, scriptDataType);
  }

  WHashedString sBaseClassHashed;
  sBaseClassHashed.Assign(sBaseClass);

  RegisterNodeType(typeDesc, std::move(nodeDesc), sBaseClassHashed);
}

void WVisualScriptNodeRegistry::CreateFunctionCallNodeType(const WRTTI* pRtti, const WHashedString& sCategory, const WAbstractFunctionProperty* pFunction, const WScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
{
  WHashSet<WStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = WDynamicCast<const WDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

  WTempHybridArray<const WFunctionArgumentAttributes*, 8> argumentAttributes;
  CollectFunctionArgumentAttributes(pFunction, argumentAttributes);

  WStringView sTypeName = StripTypeName(pRtti->GetTypeName());

  WStringView sFunctionName = pFunction->GetPropertyName();
  sFunctionName.TrimWordStart("Reflection_");

  WReflectedTypeDescriptor typeDesc;
  bool bHasTitle = false;
  {
    if (bIsEntryFunction)
    {
      WColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, &color);
    }
    else
    {
      FillDesc(typeDesc, pRtti);
    }

    WStringBuilder temp;
    temp.Set(typeDesc.m_sTypeName, "::", sFunctionName);
    typeDesc.m_sTypeName = temp;

    if (bIsEntryFunction)
    {
      AddInputProperty(typeDesc, "CoroutineMode", WGetStaticRTTI<WScriptCoroutineCreationMode>(), WVisualScriptDataType::Int64);
    }

    if (auto pTitleAttribute = pFunction->GetAttributeByType<WTitleAttribute>())
    {
      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);

      bHasTitle = true;
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pFunction);
  if (bIsEntryFunction)
  {
    nodeDesc.m_sFilterByBaseClass.Assign(sTypeName);
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::EntryCall;
  }
  else
  {
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::ReflectedFunction;
  }

  {
    if (pFunction->GetFlags().IsSet(WPropertyFlags::Const) == false)
    {
      if (bIsEntryFunction == false)
      {
        nodeDesc.AddInputExecutionPin("");
      }
      nodeDesc.AddOutputExecutionPin("");
    }

    if (bIsEntryFunction == false)
    {
      if (pFunction->GetFunctionType() == WFunctionType::Member)
      {
        // GameObject and World pins will default to the script owner's game object/world thus they are not required
        const bool bRequired = pRtti->IsDerivedFrom<WGameObject>() == false && pRtti->IsDerivedFrom<WWorld>() == false;
        nodeDesc.AddInputDataPin(sTypeName, pRtti, WVisualScriptDataType::FromRtti(pRtti), bRequired);
      }

      if (const WRTTI* pReturnRtti = pFunction->GetReturnType())
      {
        WVisualScriptDataType::Enum scriptDataType;
        if (GetScriptDataType(pReturnRtti, scriptDataType, pFunction->GetPropertyName(), "return value").Failed())
        {
          return;
        }

        m_TypesToUpdate.Insert(pReturnRtti);

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    W_ASSERT_ALWAYS(pFunction->GetArgumentCount() == pScriptableFunctionAttribute->GetArgumentCount(),
      "The function reflection for '{}::{}' does not match the actual signature. Num arguments: {}, reflected arguments: {}.", sTypeName, sFunctionName, pFunction->GetArgumentCount(), pScriptableFunctionAttribute->GetArgumentCount());

    WUInt32 titleArgIdx = WInvalidIndex;

    WStringBuilder sArgName;
    for (WUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.SetFormat("Arg{}", argIdx);

      auto pArgRtti = pFunction->GetArgumentType(argIdx);
      auto argType = pScriptableFunctionAttribute->GetArgumentType(argIdx);
      const bool bIsDynamicPinProperty = dynamicPins.Contains(sArgName);

      WHashedString sDynamicPinProperty;
      if (bIsDynamicPinProperty)
      {
        sDynamicPinProperty.Assign(sArgName);
      }

      WVisualScriptDataType::Enum scriptDataType;
      if (GetScriptDataType(pArgRtti, scriptDataType, pFunction->GetPropertyName(), sArgName).Failed())
      {
        return;
      }

      WVisualScriptDataType::Enum pinScriptDataType = scriptDataType;
      const bool bIsArrayDynamicPinProperty = bIsDynamicPinProperty && scriptDataType == WVisualScriptDataType::Array;
      if (bIsArrayDynamicPinProperty)
      {
        pArgRtti = WGetStaticRTTI<WVariant>();
        pinScriptDataType = WVisualScriptDataType::Variant;
      }

      m_TypesToUpdate.Insert(pArgRtti);

      if (bIsEntryFunction)
      {
        nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
      }
      else
      {
        if (argType == WScriptableFunctionAttribute::In || argType == WScriptableFunctionAttribute::Inout)
        {
          if (WVisualScriptDataType::IsPointer(scriptDataType) == false)
          {
            WArrayPtr<const WPropertyAttribute* const> attributes;
            if (argIdx < argumentAttributes.GetCount() && argumentAttributes[argIdx] != nullptr)
            {
              attributes = argumentAttributes[argIdx]->GetArgumentAttributes();
            }

            AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType, attributes);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty, nullptr, bIsArrayDynamicPinProperty);

          if (titleArgIdx == WInvalidIndex &&
              (pinScriptDataType == WVisualScriptDataType::String || pinScriptDataType == WVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }

        if (argType == WScriptableFunctionAttribute::Out || argType == WScriptableFunctionAttribute::Inout)
        {
          if (!pFunction->GetArgumentFlags(argIdx).IsAnySet(WPropertyFlags::Reference | WPropertyFlags::Pointer))
          {
            WLog::Error("Script function '{}::{}' argument {} is marked 'out' but is not a non-const reference or pointer value", sTypeName, sFunctionName, argIdx);
            return;
          }

          nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
        }
      }
    }

    if (bIsEntryFunction)
    {
      nodeDesc.AddOutputDataPin("CoroutineID", WGetStaticRTTI<WScriptCoroutineHandle>(), WVisualScriptDataType::Coroutine);
    }

    if (bHasTitle == false && titleArgIdx != WInvalidIndex)
    {
      WStringBuilder sTitle;
      sTitle.Set(GetTypeName(pRtti), "::", sFunctionName, " {", pScriptableFunctionAttribute->GetArgumentName(titleArgIdx), "}");

      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, sTitle);
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCategory);
}

void WVisualScriptNodeRegistry::CreateCoroutineNodeType(const WRTTI* pRtti)
{
  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
    return;

  const WAbstractFunctionProperty* pStartFunc = nullptr;
  const WScriptableFunctionAttribute* pScriptableFuncAttribute = nullptr;
  for (auto pFunc : pRtti->GetFunctions())
  {
    if (WStringUtils::IsEqual(pFunc->GetPropertyName(), "Start"))
    {
      if (auto pAttr = pFunc->GetAttributeByType<WScriptableFunctionAttribute>())
      {
        pStartFunc = pFunc;
        pScriptableFuncAttribute = pAttr;
        break;
      }
    }
  }

  if (pStartFunc == nullptr || pScriptableFuncAttribute == nullptr)
  {
    WLog::Warning("The script coroutine '{}' has no reflected script function called 'Start'.", pRtti->GetTypeName());
    return;
  }

  WReflectedTypeDescriptor typeDesc;
  {
    const WColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::Coroutine);
    FillDesc(typeDesc, pRtti, &coroutineColor);

    WStringBuilder temp;
    temp.Set("Coroutine::", typeDesc.m_sTypeName);
    typeDesc.m_sTypeName = temp;

    if (auto pTitleAttribute = pRtti->GetAttributeByType<WTitleAttribute>())
    {
      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pStartFunc);
  nodeDesc.m_Type = WVisualScriptNodeDescription::Type::InplaceCoroutine;

  nodeDesc.AddInputExecutionPin("");
  nodeDesc.AddOutputExecutionPin("Succeeded");
  nodeDesc.AddOutputExecutionPin("Failed");

  WStringBuilder sArgName;
  for (WUInt32 argIdx = 0; argIdx < pStartFunc->GetArgumentCount(); ++argIdx)
  {
    sArgName = pScriptableFuncAttribute->GetArgumentName(argIdx);
    if (sArgName.IsEmpty())
      sArgName.SetFormat("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != WScriptableFunctionAttribute::In)
    {
      // WLog::Error("Script function out parameter are not yet supported");
      return;
    }

    WVisualScriptDataType::Enum scriptDataType = WVisualScriptDataType::Invalid;
    if (GetScriptDataType(pArgRtti, scriptDataType, pStartFunc->GetPropertyName(), sArgName).Failed())
    {
      return;
    }

    if (WVisualScriptDataType::IsPointer(scriptDataType) == false)
    {
      AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType);
    }

    nodeDesc.AddInputDataPin(sArgName, pArgRtti, scriptDataType, false);
  }

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
}

void WVisualScriptNodeRegistry::CreateMessageNodeTypes(const WRTTI* pRtti)
{
  if (pRtti == WGetStaticRTTI<WMessage>() ||
      pRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
    return;

  WStringView sTypeName = GetTypeName(pRtti);

  // Message Handler
  {
    WReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      WStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", sTypeName);
      typeDesc.m_sTypeName = temp;

      AddInputProperty(typeDesc, "CoroutineMode", WGetStaticRTTI<WScriptCoroutineCreationMode>(), WVisualScriptDataType::Int64);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::MessageHandler;

    nodeDesc.AddOutputExecutionPin("");

    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      auto pPropRtti = pProp->GetSpecificType();
      WVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == WVisualScriptDataType::Invalid)
        continue;

      nodeDesc.AddOutputDataPin(pProp->GetPropertyName(), pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    nodeDesc.AddOutputDataPin("CoroutineID", WGetStaticRTTI<WScriptCoroutineHandle>(), WVisualScriptDataType::Coroutine);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEventHandlerCategory);
  }

  // Message Sender
  {
    WReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      WStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", sTypeName);
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", sTypeName, " {Delay}");
      auto pAttr = W_DEFAULT_NEW(WTitleAttribute, temp);
      typeDesc.m_Attributes.PushBack(pAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::SendMessage;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("GameObject", WGetStaticRTTI<WGameObject>(), WVisualScriptDataType::GameObject, false);
    nodeDesc.AddInputDataPin("Component", WGetStaticRTTI<WComponent>(), WVisualScriptDataType::Component, false);
    AddInputDataPin<WVisualScriptSendMessageMode>(typeDesc, nodeDesc, "SendMode");
    AddInputDataPin<WTime>(typeDesc, nodeDesc, "Delay");

    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
        continue;

      auto szPropName = pProp->GetPropertyName();
      auto pPropRtti = pProp->GetSpecificType();
      WVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == WVisualScriptDataType::Invalid)
        continue;

      if (WVisualScriptDataType::IsPointer(scriptDataType) == false)
      {
        AddInputProperty(typeDesc, szPropName, pPropRtti, scriptDataType);
      }

      nodeDesc.AddInputDataPin(szPropName, pPropRtti, scriptDataType, false);
      nodeDesc.AddOutputDataPin(szPropName, pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMessagesCategory);
  }
}

void WVisualScriptNodeRegistry::CreateEnumNodeTypes(const WRTTI* pRtti)
{
  if (m_ExposedTypes.Insert(pRtti))
    return;

  WStringView sTypeName = GetTypeName(pRtti);
  WColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(WVisualScriptDataType::EnumValue);

  // Value
  {
    WStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    WReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, WVisualScriptDataType::EnumValue);

    WStringBuilder sTitle;
    sTitle.Set(sTypeName, "::{Value}");

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, WVisualScriptDataType::EnumValue);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }

  // Switch
  {
    WStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    WReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);

    WStringBuilder sTitle;
    sTitle.Set(sTypeName, "::Switch");

    auto pAttr = W_DEFAULT_NEW(WTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = WVisualScriptNodeDescription::Type::Builtin_Switch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddInputDataPin("Value", pRtti, WVisualScriptDataType::EnumValue, false);

    WTempHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
    WReflectionUtils::GetEnumKeysAndValues(pRtti, enumKeysAndValues, WReflectionUtils::EnumConversionMode::ValueNameOnly);
    for (auto& keyAndValue : enumKeysAndValues)
    {
      nodeDesc.AddOutputExecutionPin(keyAndValue.m_sKey);
    }

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }
}

void WVisualScriptNodeRegistry::FillDesc(WReflectedTypeDescriptor& desc, const WRTTI* pRtti, const WColorGammaUB* pColorOverride /*= nullptr */)
{
  WStringBuilder sTypeName = GetTypeName(pRtti);
  const WRTTI* pBaseClass = FindTopMostBaseClass(pRtti);

  WColorGammaUB color;
  if (pColorOverride == nullptr)
  {
    if (pBaseClass != pRtti)
    {
      color = NiceColorFromName(sTypeName, StripTypeName(pBaseClass->GetTypeName()));
    }
    else
    {
      auto scriptDataType = WVisualScriptDataType::FromRtti(pRtti);
      if (scriptDataType != WVisualScriptDataType::Invalid &&
          scriptDataType != WVisualScriptDataType::Component &&
          scriptDataType != WVisualScriptDataType::TypedPointer)
      {
        color = PinDesc::GetColorForScriptDataType(scriptDataType);
      }
      else
      {
        color = NiceColorFromName(sTypeName);
      }
    }
  }
  else
  {
    color = *pColorOverride;
  }

  FillDesc(desc, sTypeName, color);
}

void WVisualScriptNodeRegistry::FillDesc(WReflectedTypeDescriptor& desc, WStringView sTypeName, const WColorGammaUB& color)
{
  WStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc = {};
  desc.m_sTypeName = sTypeNameFull;
  desc.m_sPluginName = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = WTypeFlags::Class;

  // Color
  {
    auto pAttr = W_DEFAULT_NEW(WColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}

const WRTTI* WVisualScriptNodeRegistry::RegisterNodeType(WReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const WHashedString& sCategory)
{
  const WRTTI* pRtti = WPhantomRttiManager::RegisterType(typeDesc);
  if (m_TypeToNodeDescs.Insert(pRtti, std::move(nodeDesc)) == false)
  {
    auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
    nodeTemplate.m_pType = pRtti;
    nodeTemplate.m_sCategory = sCategory;
  }

  return pRtti;
}

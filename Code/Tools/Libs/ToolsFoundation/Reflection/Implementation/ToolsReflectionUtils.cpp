#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  struct GetDoubleFunc
  {
    GetDoubleFunc(const WVariant& value)
      : m_Value(value)
    {
    }
    template <typename T>
    void operator()()
    {
      if (m_Value.CanConvertTo<double>())
      {
        m_fValue = m_Value.ConvertTo<double>();
        m_bValid = true;
      }
    }

    const WVariant& m_Value;
    double m_fValue = 0;
    bool m_bValid = false;
  };

  template <>
  void GetDoubleFunc::operator()<WAngle>()
  {
    m_fValue = m_Value.Get<WAngle>().GetDegree();
    m_bValid = true;
  }

  template <>
  void GetDoubleFunc::operator()<WTime>()
  {
    m_fValue = m_Value.Get<WTime>().GetSeconds();
    m_bValid = true;
  }

  struct GetVariantFunc
  {
    GetVariantFunc(double fValue, WVariantType::Enum type, WVariant& out_value)
      : m_fValue(fValue)
      , m_Type(type)
      , m_Value(out_value)
    {
    }
    template <typename T>
    void operator()()
    {
      m_Value = m_fValue;
      if (m_Value.CanConvertTo(m_Type))
      {
        m_Value = m_Value.ConvertTo(m_Type);
        m_bValid = true;
      }
      else
      {
        m_Value = WVariant();
      }
    }

    double m_fValue;
    WVariantType::Enum m_Type;
    WVariant& m_Value;
    bool m_bValid = false;
  };

  template <>
  void GetVariantFunc::operator()<WAngle>()
  {
    m_Value = WAngle::MakeFromDegree((float)m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<WTime>()
  {
    m_Value = WTime::MakeFromSeconds(m_fValue);
    m_bValid = true;
  }
} // namespace
////////////////////////////////////////////////////////////////////////
// WToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

WVariantType::Enum WToolsReflectionUtils::GetStorageType(const WAbstractProperty* pProperty)
{
  WVariantType::Enum type = WVariantType::Uuid;

  const bool bIsValueType = WReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (bIsValueType)
        type = pProperty->GetSpecificType()->GetVariantType();
      else if (pProperty->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        type = WVariantType::Int64;
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      type = WVariantType::VariantArray;
    }
    break;
    case WPropertyCategory::Map:
    {
      type = WVariantType::VariantDictionary;
    }
    break;
    default:
      break;
  }

  // We can't 'store' a string view as it has no ownership of its own. Thus, all string views are stored as strings instead.
  if (type == WVariantType::StringView)
    type = WVariantType::String;

  return type;
}

WVariant WToolsReflectionUtils::GetStorageDefault(const WAbstractProperty* pProperty)
{
  const WDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<WDefaultValueAttribute>();
  const bool bIsValueType = WReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      const WVariantType::Enum memberType = GetStorageType(pProperty);
      WVariant value = WReflectionUtils::GetDefaultValue(pProperty);
      // Sometimes, the default value does not match the storage type, e.g. WStringView is stored as WString as it needs to be stored in the editor representation, but the reflection can still return default values matching WStringView (constants for example).
      if (bIsValueType && value.GetType() != memberType)
        value = value.ConvertTo(memberType);

      W_ASSERT_DEBUG(!value.IsValid() || memberType == value.GetType(), "Default value type does not match the storage type of the property");
      return value;
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      if (bIsValueType && pAttrib && pAttrib->GetValue().IsA<WVariantArray>())
      {
        auto elementType = pProperty->GetFlags().IsSet(WPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : WVariantType::Uuid;

        const WVariantArray& value = pAttrib->GetValue().Get<WVariantArray>();
        WVariantArray ret;
        ret.SetCount(value.GetCount());
        for (WUInt32 i = 0; i < value.GetCount(); i++)
        {
          ret[i] = value[i].ConvertTo(elementType);
        }
        return ret;
      }
      return WVariantArray();
    }
    break;
    case WPropertyCategory::Map:
    {
      return WVariantDictionary();
    }
    break;
    case WPropertyCategory::Constant:
    case WPropertyCategory::Function:
      break; // no defaults
  }
  return WVariant();
}

bool WToolsReflectionUtils::GetFloatFromVariant(const WVariant& val, double& out_fValue)
{
  if (val.IsValid())
  {
    GetDoubleFunc func(val);
    WVariant::DispatchTo(func, val.GetType());
    out_fValue = func.m_fValue;
    return func.m_bValid;
  }
  return false;
}


bool WToolsReflectionUtils::GetVariantFromFloat(double fValue, WVariantType::Enum type, WVariant& out_val)
{
  GetVariantFunc func(fValue, type, out_val);
  WVariant::DispatchTo(func, type);

  return func.m_bValid;
}

void WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const WRTTI* pRtti, WReflectedTypeDescriptor& out_desc)
{
  GetMinimalReflectedTypeDescriptorFromRtti(pRtti, out_desc);
  out_desc.m_Flags.Remove(WTypeFlags::Minimal);

  auto rttiProps = pRtti->GetProperties();
  const WUInt32 uiCount = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);
  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    const WAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
      case WPropertyCategory::Constant:
      {
        auto constantProp = static_cast<const WAbstractConstantProperty*>(prop);
        const WRTTI* pPropRtti = constantProp->GetSpecificType();
        if (WReflectionUtils::IsBasicType(pPropRtti))
        {
          WVariant value = constantProp->GetConstant();
          W_ASSERT_DEV(pPropRtti->GetVariantType() == value.GetType(), "Variant value type and property type should always match!");
          out_desc.m_Properties.PushBack(WReflectedPropertyDescriptor(constantProp->GetPropertyName(), value, prop->GetAttributes()));
        }
        else
        {
          W_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;

      case WPropertyCategory::Member:
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        const WRTTI* pPropRtti = prop->GetSpecificType();
        WBitflags<WPropertyFlags> flags = prop->GetFlags();
        flags.Remove(WPropertyFlags::Phantom);
        out_desc.m_Properties.PushBack(WReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(), flags, prop->GetAttributes()));
      }
      break;

      case WPropertyCategory::Function:
        break;

      default:
        break;
    }
  }

  auto rttiFunc = pRtti->GetFunctions();
  const WUInt32 uiFuncCount = rttiFunc.GetCount();
  out_desc.m_Functions.Reserve(uiFuncCount);

  for (WUInt32 i = 0; i < uiFuncCount; ++i)
  {
    const WAbstractFunctionProperty* prop = rttiFunc[i];
    WBitflags<WPropertyFlags> funcFlags = prop->GetFlags();
    funcFlags.Remove(WPropertyFlags::Phantom);
    out_desc.m_Functions.PushBack(WReflectedFunctionDescriptor(prop->GetPropertyName(), funcFlags, prop->GetFunctionType(), prop->GetAttributes()));
    WReflectedFunctionDescriptor& desc = out_desc.m_Functions.PeekBack();
    desc.m_ReturnValue = WFunctionArgumentDescriptor(prop->GetReturnType() ? prop->GetReturnType()->GetTypeName() : "", prop->GetReturnFlags());
    const WUInt32 uiArguments = prop->GetArgumentCount();
    desc.m_Arguments.Reserve(uiArguments);
    for (WUInt32 a = 0; a < uiArguments; ++a)
    {
      desc.m_Arguments.PushBack(WFunctionArgumentDescriptor(prop->GetArgumentType(a)->GetTypeName(), prop->GetArgumentFlags(a)));
    }
  }

  out_desc.m_ReferenceAttributes = pRtti->GetAttributes();
}


void WToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(const WRTTI* pRtti, WReflectedTypeDescriptor& out_desc)
{
  W_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
  out_desc.m_Flags = pRtti->GetTypeFlags() | WTypeFlags::Minimal;
  // Phantom describes how a type is represented in the current process, not the type itself: it is added by
  // WPhantomRTTI and the WPhantom*Property classes when a descriptor is registered, so it must not travel
  // with the descriptor (it would be written into every document that references a phantom type).
  out_desc.m_Flags.Remove(WTypeFlags::Phantom);
  out_desc.m_uiTypeVersion = pRtti->GetTypeVersion();
  const WRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
  out_desc.m_Functions.Clear();
  out_desc.m_Attributes.Clear();
  out_desc.m_ReferenceAttributes = WArrayPtr<WPropertyAttribute* const>();
}

static void GatherObjectTypesInternal(const WDocumentObject* pObject, WSet<const WRTTI*>& inout_types)
{
  inout_types.Insert(pObject->GetTypeAccessor().GetType());
  WReflectionUtils::GatherDependentTypes(pObject->GetTypeAccessor().GetType(), inout_types);

  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    GatherObjectTypesInternal(pChild, inout_types);
  }
}

void WToolsReflectionUtils::GatherObjectTypes(const WDocumentObject* pObject, WSet<const WRTTI*>& inout_types)
{
  GatherObjectTypesInternal(pObject, inout_types);
}

bool WToolsReflectionUtils::DependencySortTypeDescriptorArray(WDynamicArray<WReflectedTypeDescriptor*>& ref_descriptors)
{
  WMap<WReflectedTypeDescriptor*, WSet<WString>> dependencies;

  WSet<WString> typesInArray;
  // Gather all types in array
  for (WReflectedTypeDescriptor* desc : ref_descriptors)
  {
    typesInArray.Insert(desc->m_sTypeName);
  }

  // Find all direct dependencies to types in the array for each type.
  for (WReflectedTypeDescriptor* desc : ref_descriptors)
  {
    auto it = dependencies.Insert(desc, WSet<WString>());

    if (typesInArray.Contains(desc->m_sParentTypeName))
    {
      it.Value().Insert(desc->m_sParentTypeName);
    }
    for (WReflectedPropertyDescriptor& propDesc : desc->m_Properties)
    {
      if (typesInArray.Contains(propDesc.m_sType))
      {
        it.Value().Insert(propDesc.m_sType);
      }
    }
  }

  WSet<WString> accu;
  WDynamicArray<WReflectedTypeDescriptor*> sorted;
  sorted.Reserve(ref_descriptors.GetCount());
  // Build new sorted types array.
  while (!ref_descriptors.IsEmpty())
  {
    bool bDeadEnd = true;
    for (WReflectedTypeDescriptor* desc : ref_descriptors)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        ref_descriptors.RemoveAndCopy(desc);
        accu.Insert(desc->m_sTypeName);
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  ref_descriptors = sorted;
  return true;
}

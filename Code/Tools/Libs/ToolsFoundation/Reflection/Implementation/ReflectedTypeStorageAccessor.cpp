#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

////////////////////////////////////////////////////////////////////////
// WReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

WReflectedTypeStorageAccessor::WReflectedTypeStorageAccessor(const WRTTI* pRtti, WDocumentObject* pOwner)
  : WIReflectedTypeAccessor(pRtti, pOwner)
{
  const WRTTI* pType = pRtti;
  W_ASSERT_DEV(pType != nullptr, "Trying to construct an WReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = WReflectedTypeStorageManager::AddStorageAccessor(this);
  W_ASSERT_DEV(m_pMapping != nullptr, "The type for this WReflectedTypeStorageAccessor is unknown to the WReflectedTypeStorageManager!");

  auto& indexTable = m_pMapping->m_PathToStorageInfoTable;
  const WUInt32 uiProperties = indexTable.GetCount();
  // To prevent re-allocs due to new properties being added we reserve 20% more space.
  m_Data.Reserve(uiProperties + uiProperties / 20);
  m_Data.SetCount(uiProperties);

  // Fill data storage with default values for the given types.
  for (auto it = indexTable.GetIterator(); it.IsValid(); ++it)
  {
    const auto& storageInfo = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

WReflectedTypeStorageAccessor::~WReflectedTypeStorageAccessor()
{
  WReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const WVariant WReflectedTypeStorageAccessor::GetValue(WStringView sProperty, WVariant index, WStatus* pRes) const
{
  const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = WStatus(WFmt("Property '{0}' not found in type '{1}'", sProperty, GetType()->GetTypeName()));
    return WVariant();
  }

  if (pRes)
    *pRes = WStatus(W_SUCCESS);
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
        if (index.IsValid())
        {
          if (pRes)
          {
            *pRes = WStatus(WFmt("Property '{0}' is a member property but an index of '{1}' is given", sProperty, index));
          }
          return WVariant();
        }
        return m_Data[storageInfo->m_uiIndex];
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetValue(index, pRes);
      }
      break;
      default:
        break;
    }
  }
  return WVariant();
}

bool WReflectedTypeStorageAccessor::SetValue(WStringView sProperty, const WVariant& value, WVariant index)
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;
    W_ASSERT_DEV(pProp->GetSpecificType() == WGetStaticRTTI<WVariant>() || value.IsValid(), "");

    if (storageInfo->m_Type == WVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = WReflectionUtils::IsValueType(pProp);
    const WVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(WPropertyFlags::Class) && !isValueType) ? WVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        if (index.IsValid())
          return false;

        if (value.IsA<WString>() && pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        {
          WInt64 iValue;
          WReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<WString>(), iValue);
          m_Data[storageInfo->m_uiIndex] = WVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
        {
          m_Data[storageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(storageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
          return true;
        }
      }
      break;
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      case WPropertyCategory::Map:
      {
        if (index.IsA<WString>())
        {
          if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

WInt32 WReflectedTypeStorageAccessor::GetCount(WStringView sProperty) const
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == WVariant::Type::Invalid)
      return false;

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
        return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetCount();
      default:
        break;
    }
  }
  return -1;
}

bool WReflectedTypeStorageAccessor::GetKeys(WStringView sProperty, WDynamicArray<WVariant>& out_keys) const
{
  out_keys.Clear();

  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == WVariant::Type::Invalid)
      return false;

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetKeys(out_keys).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}
bool WReflectedTypeStorageAccessor::InsertValue(WStringView sProperty, WVariant index, const WVariant& value)
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == WVariant::Type::Invalid)
      return false;

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    if (storageInfo->m_Type == WVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = WReflectionUtils::IsValueType(pProp);
    const WVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(WPropertyFlags::Class) && !isValueType) ? WVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      case WPropertyCategory::Map:
      {
        if (index.IsA<WString>())
        {
          if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool WReflectedTypeStorageAccessor::RemoveValue(WStringView sProperty, WVariant index)
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == WVariant::Type::Invalid)
      return false;

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).RemoveValue(index).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool WReflectedTypeStorageAccessor::MoveValue(WStringView sProperty, WVariant oldIndex, WVariant newIndex)
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == WVariant::Type::Invalid)
      return false;

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        return WVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).MoveValue(oldIndex, newIndex).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

WVariant WReflectedTypeStorageAccessor::GetPropertyChildIndex(WStringView sProperty, const WVariant& value) const
{
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    //    if (storageInfo->m_Type == WVariant::Type::Invalid)
    //      return WVariant();

    const WAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return WVariant();

    const bool isValueType = WReflectionUtils::IsValueType(pProp);
    const WVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(WPropertyFlags::Class) && !isValueType) ? WVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const WVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<WVariantArray>();
          for (WUInt32 i = 0; i < values.GetCount(); i++)
          {
            if (values[i] == value)
              return WVariant((WUInt32)i);
          }
        }
      }
      break;
      case WPropertyCategory::Map:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const WVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<WVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return WVariant(it.Key());
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return WVariant();
}

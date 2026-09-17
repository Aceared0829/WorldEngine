#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVariantSubAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WVariantSubAccessor::WVariantSubAccessor(WObjectAccessorBase* pSource, const WAbstractProperty* pProp)
  : WObjectProxyAccessor(pSource)
  , m_pProp(pProp)
{
}

void WVariantSubAccessor::SetSubItems(const WMap<const WDocumentObject*, WVariant>& subItemMap)
{
  m_SubItemMap = subItemMap;
}

WInt32 WVariantSubAccessor::GetDepth() const
{
  if (auto variantSubAccessor = WDynamicCast<WVariantSubAccessor*>(GetSourceAccessor()))
  {
    return variantSubAccessor->GetDepth() + 1;
  }
  return 1;
}

WResult WVariantSubAccessor::GetPath(const WDocumentObject* pObject, WDynamicArray<WVariant>& out_path) const
{
  out_path.Clear();
  if (auto variantSubAccessor = WDynamicCast<WVariantSubAccessor*>(GetSourceAccessor()))
  {
    W_SUCCEED_OR_RETURN(variantSubAccessor->GetPath(pObject, out_path));
  }
  WVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return W_FAILURE;

  out_path.PushBack(subItem);
  return W_SUCCESS;
}

WStatus WVariantSubAccessor::GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index)
{
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  WStatus result(W_SUCCESS);
  out_value = WVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

WStatus WVariantSubAccessor::SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

WStatus WVariantSubAccessor::InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

WStatus WVariantSubAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

WStatus WVariantSubAccessor::MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

WStatus WVariantSubAccessor::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = WVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return W_SUCCESS;
}

WStatus WVariantSubAccessor::GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

WStatus WVariantSubAccessor::GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  WTempHybridArray<WVariant, 16> keys;
  WVariantStorageAccessor accessor(pProp->GetPropertyName(), subValue);
  W_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const WVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return W_SUCCESS;
}

WObjectAccessorBase* WVariantSubAccessor::ResolveProxy(const WDocumentObject*& ref_pObject, const WRTTI*& ref_pType, const WAbstractProperty*& ref_pProp, WDynamicArray<WVariant>& ref_indices)
{
  WVariant subItem;
  if (m_SubItemMap.TryGetValue(ref_pObject, subItem))
  {
    ref_indices.InsertAt(0, subItem);
  }

  return m_pSource->ResolveProxy(ref_pObject, ref_pType, ref_pProp, ref_indices);
}

WStatus WVariantSubAccessor::GetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value)
{
  WStatus result = WObjectProxyAccessor::GetValue(pObject, pProp, out_value);
  if (result.Failed())
    return result;

  WVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return WStatus(WFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  out_value = WVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(subItem, &result);
  return result;
}

WStatus WVariantSubAccessor::SetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WStatus(WVariant&)>& func)
{
  W_ASSERT_DEBUG(m_pProp == pProp, "WVariantSubAccessor should only be used to access a single variant property");
  WVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return WStatus(WFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  WVariant currentValue;
  W_SUCCEED_OR_RETURN(WObjectProxyAccessor::GetValue(pObject, pProp, currentValue, subItem));
  W_SUCCEED_OR_RETURN(func(currentValue));
  return WObjectProxyAccessor::SetValue(pObject, pProp, currentValue, subItem);
}

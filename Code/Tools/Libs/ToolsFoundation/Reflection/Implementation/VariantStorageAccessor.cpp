#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

WVariantStorageAccessor::WVariantStorageAccessor(WStringView sProperty, WVariant& value)
  : m_sProperty(sProperty)
  , m_Value(value)
{
}

WVariantStorageAccessor::WVariantStorageAccessor(WStringView sProperty, const WVariant& value)
  : m_sProperty(sProperty)
  , m_Value(const_cast<WVariant&>(value))
{
}

WVariant WVariantStorageAccessor::GetValue(WVariant index, WStatus* pRes) const
{
  if (!index.IsValid())
    return m_Value;

  if (index.IsNumber())
  {
    if (!m_Value.IsA<WVariantArray>())
    {
      if (pRes)
        *pRes = WStatus(WFmt("Index '{0}' for property '{1}' is invalid as the property is not an array.", index, m_sProperty));
      return WVariant();
    }
    const WVariantArray& values = m_Value.Get<WVariantArray>();
    WUInt32 uiIndex = index.ConvertTo<WUInt32>();
    if (uiIndex < values.GetCount())
    {
      return values[uiIndex];
    }
  }
  else if (index.IsA<WString>())
  {
    if (!m_Value.IsA<WVariantDictionary>())
    {
      if (pRes)
        *pRes = WStatus(WFmt("Index '{0}' for property '{1}' is invalid as the property is not a dictionary.", index, m_sProperty));
      return WVariant();
    }
    const WVariantDictionary& values = m_Value.Get<WVariantDictionary>();
    const WString& sIndex = index.Get<WString>();
    if (const WVariant* pValue = values.GetValue(sIndex))
    {
      return *pValue;
    }
  }

  if (pRes)
    *pRes = WStatus(WFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, m_sProperty));
  return WVariant();
}

WStatus WVariantStorageAccessor::SetValue(const WVariant& value, WVariant index)
{
  if (!index.IsValid())
  {
    m_Value = value;
    return W_SUCCESS;
  }

  if (index.IsNumber() && m_Value.IsA<WVariantArray>())
  {
    WVariantArray& values = m_Value.GetWritable<WVariantArray>();
    WUInt32 uiIndex = index.ConvertTo<WUInt32>();
    if (uiIndex >= values.GetCount())
    {
      return WStatus(WFmt("Index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));
    }
    values[uiIndex] = value;
    return W_SUCCESS;
  }
  else if (index.IsA<WString>() && m_Value.IsA<WVariantDictionary>())
  {
    WVariantDictionary& values = m_Value.GetWritable<WVariantDictionary>();
    const WString& sIndex = index.Get<WString>();
    if (!values.Contains(sIndex))
    {
      return WStatus(WFmt("Index '{0}' for property '{1}' is out of bounds.", sIndex, m_sProperty));
    }
    values[sIndex] = value;
    return W_SUCCESS;
  }
  return WStatus(WFmt("Index '{0}' for property '{1}' is invalid.", index, m_sProperty));
}

WInt32 WVariantStorageAccessor::GetCount() const
{
  if (m_Value.IsA<WVariantArray>())
    return m_Value.Get<WVariantArray>().GetCount();
  else if (m_Value.IsA<WVariantDictionary>())
    return m_Value.Get<WVariantDictionary>().GetCount();
  return 0;
}

WStatus WVariantStorageAccessor::GetKeys(WDynamicArray<WVariant>& out_keys) const
{
  if (m_Value.IsA<WVariantArray>())
  {
    const WVariantArray& values = m_Value.Get<WVariantArray>();
    out_keys.Reserve(values.GetCount());
    for (WUInt32 i = 0; i < values.GetCount(); ++i)
    {
      out_keys.PushBack(i);
    }
    return W_SUCCESS;
  }
  else if (m_Value.IsA<WVariantDictionary>())
  {
    const WVariantDictionary& values = m_Value.Get<WVariantDictionary>();
    out_keys.Reserve(values.GetCount());
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(WVariant(it.Key()));
    }
    return W_SUCCESS;
  }
  return WStatus(WFmt("Property '{0}' is not a container.", m_sProperty));
}

WStatus WVariantStorageAccessor::InsertValue(const WVariant& index, const WVariant& value)
{
  if (index.IsNumber() && m_Value.IsA<WVariantArray>())
  {
    WVariantArray& values = m_Value.GetWritable<WVariantArray>();
    WInt32 iIndex = index.ConvertTo<WInt32>();
    const WInt32 iCount = (WInt32)values.GetCount();
    if (iIndex == -1)
    {
      iIndex = iCount;
    }
    if (iIndex > iCount)
      return WStatus(WFmt("InsertValue: index '{0}' for property '{1}' is out of bounds.", iIndex, m_sProperty));

    values.InsertAt(iIndex, value);
    return W_SUCCESS;
  }
  else if (index.IsA<WString>() && m_Value.IsA<WVariantDictionary>())
  {
    WVariantDictionary& values = m_Value.GetWritable<WVariantDictionary>();
    const WString& sIndex = index.Get<WString>();
    if (values.Contains(index.Get<WString>()))
      return WStatus(WFmt("InsertValue: index '{0}' for property '{1}' already exists.", sIndex, m_sProperty));

    values.Insert(sIndex, value);
    return W_SUCCESS;
  }
  return WStatus(WFmt("InsertValue: Property '{0}' is not a container or index {1} is invalid.", m_sProperty, index));
}

WStatus WVariantStorageAccessor::RemoveValue(const WVariant& index)
{
  if (index.IsNumber() && m_Value.IsA<WVariantArray>())
  {
    WVariantArray& values = m_Value.GetWritable<WVariantArray>();
    const WUInt32 uiIndex = index.ConvertTo<WUInt32>();

    // '>=', not '>': index == GetCount() addresses one past the last element, which is a valid
    // position to insert at but not one to remove from. With '>' it reached RemoveAtAndCopy() and
    // read out of bounds.
    if (uiIndex >= values.GetCount())
      return WStatus(WFmt("RemoveValue: index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));

    values.RemoveAtAndCopy(uiIndex);
    return W_SUCCESS;
  }
  else if (index.IsA<WString>() && m_Value.IsA<WVariantDictionary>())
  {
    WVariantDictionary& values = m_Value.GetWritable<WVariantDictionary>();
    const WString& sIndex = index.Get<WString>();
    if (!values.Contains(index.Get<WString>()))
      return WStatus(WFmt("RemoveValue: index '{0}' for property '{1}' does not exists.", sIndex, m_sProperty));

    values.Remove(sIndex);
    return W_SUCCESS;
  }
  return WStatus(WFmt("RemoveValue: Property '{0}' is not a container or index '{1}' is invalid.", m_sProperty, index));
}

WStatus WVariantStorageAccessor::MoveValue(const WVariant& oldIndex, const WVariant& newIndex)
{
  if (m_Value.IsA<WVariantArray>() && oldIndex.IsNumber() && newIndex.IsNumber())
  {
    WVariantArray& values = m_Value.GetWritable<WVariantArray>();
    WUInt32 uiOldIndex = oldIndex.ConvertTo<WUInt32>();
    WUInt32 uiNewIndex = newIndex.ConvertTo<WUInt32>();
    if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
    {
      WVariant value = values[uiOldIndex];
      values.RemoveAtAndCopy(uiOldIndex);
      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }
      values.InsertAt(uiNewIndex, value);
      return W_SUCCESS;
    }
    else
    {
      return WStatus(WFmt("MoveValue: index '{0}' or '{1}' for property '{2}' is out of bounds.", uiOldIndex, uiNewIndex, m_sProperty));
    }
  }
  else if (m_Value.IsA<WVariantDictionary>() && oldIndex.IsA<WString>() && newIndex.IsA<WString>())
  {
    WVariantDictionary& values = m_Value.GetWritable<WVariantDictionary>();
    const WString& sOldIndex = oldIndex.Get<WString>();
    const WString& sNewIndex = newIndex.Get<WString>();

    if (!values.Contains(sOldIndex))
      return WStatus(WFmt("MoveValue: old index '{0}' for property '{2}' does not exist.", sOldIndex, m_sProperty));
    else if (values.Contains(sNewIndex))
      return WStatus(WFmt("MoveValue: new index '{0}' for property '{2}' already exists.", sNewIndex, m_sProperty));

    values.Insert(sNewIndex, values[sOldIndex]);
    values.Remove(sOldIndex);
    return W_SUCCESS;
  }
  return WStatus(WFmt("MoveValue: Property '{0}' is not a container or index '{1}' or '{2}' is invalid.", m_sProperty, oldIndex, newIndex));
}

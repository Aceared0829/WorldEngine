#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

WIReflectedTypeAccessor& WDocumentObject::GetTypeAccessor()
{
  const WDocumentObject* pMe = this;
  return const_cast<WIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

WUInt32 WDocumentObject::GetChildIndex(const WDocumentObject* pChild) const
{
  return m_Children.IndexOf(const_cast<WDocumentObject*>(pChild));
}

void WDocumentObject::InsertSubObject(WDocumentObject* pObject, WStringView sProperty, const WVariant& index)
{
  W_ASSERT_DEV(pObject != nullptr, "");
  W_ASSERT_DEV(!sProperty.IsEmpty(), "Child objects must have a parent property to insert into");
  WIReflectedTypeAccessor& accessor = GetTypeAccessor();

  const WRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(sProperty);
  W_ASSERT_DEV(pProp && pProp->GetFlags().IsSet(WPropertyFlags::Class) &&
                  (!pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)),
    "Only class type or pointer to class type that own the object can be inserted, everything else is handled by value.");

  if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
  {
    if (!index.IsValid() || (index.CanConvertTo<WInt32>() && index.ConvertTo<WInt32>() == -1))
    {
      WVariant newIndex = accessor.GetCount(sProperty);
      bool bRes = accessor.InsertValue(sProperty, newIndex, pObject->GetGuid());
      W_ASSERT_DEV(bRes, "");
    }
    else
    {
      bool bRes = accessor.InsertValue(sProperty, index, pObject->GetGuid());
      W_ASSERT_DEV(bRes, "");
    }
  }
  else if (pProp->GetCategory() == WPropertyCategory::Map)
  {
    W_ASSERT_DEV(index.IsA<WString>(), "Map key must be a string.");
    bool bRes = accessor.InsertValue(sProperty, index, pObject->GetGuid());
    W_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == WPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(sProperty, pObject->GetGuid());
    W_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_sParentProperty = sProperty;
  pObject->m_pParent = this;
  m_Children.PushBack(pObject);
}

void WDocumentObject::RemoveSubObject(WDocumentObject* pObject)
{
  W_ASSERT_DEV(pObject != nullptr, "");
  W_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  W_ASSERT_DEV(this == pObject->m_pParent, "");
  WIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const WRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(pObject->m_sParentProperty);
  if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set ||
      pProp->GetCategory() == WPropertyCategory::Map)
  {
    WVariant index = accessor.GetPropertyChildIndex(pObject->m_sParentProperty, pObject->GetGuid());
    bool bRes = accessor.RemoveValue(pObject->m_sParentProperty, index);
    W_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == WPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(pObject->m_sParentProperty, WUuid());
    W_ASSERT_DEV(bRes, "");
  }

  m_Children.RemoveAndCopy(pObject);
  pObject->m_pParent = nullptr;
}

void WDocumentObject::ComputeObjectHash(WUInt64& ref_uiHash) const
{
  const WIReflectedTypeAccessor& acc = GetTypeAccessor();
  auto pType = acc.GetType();

  ref_uiHash = WHashingUtils::xxHash64(&m_Guid, sizeof(WUuid), ref_uiHash);
  HashPropertiesRecursive(acc, ref_uiHash, pType);
}


WDocumentObject* WDocumentObject::GetChild(const WUuid& guid)
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}


const WDocumentObject* WDocumentObject::GetChild(const WUuid& guid) const
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}

const WAbstractProperty* WDocumentObject::GetParentPropertyType() const
{
  if (!m_pParent)
    return nullptr;
  const WIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  const WRTTI* pType = accessor.GetType();
  return pType->FindPropertyByName(m_sParentProperty);
}

WVariant WDocumentObject::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return WVariant();
  const WIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
}

bool WDocumentObject::IsOnHeap() const
{
  /// \todo Christopher: This crashes when the pointer is nullptr, which appears to be possible
  /// It happened for me when duplicating (CTRL+D) 2 objects 2 times then moving them and finally undoing everything
  W_ASSERT_DEV(m_pParent != nullptr,
    "Object being modified is not part of the document, e.g. may be in the undo stack instead. "
    "This could happen if within an undo / redo op some callback tries to create a new undo scope / update prefabs etc.");

  if (GetParent() == GetDocumentObjectManager()->GetRootObject())
    return true;

  auto* pProp = GetParentPropertyType();
  return pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner);
}


void WDocumentObject::HashPropertiesRecursive(const WIReflectedTypeAccessor& acc, WUInt64& uiHash, const WRTTI* pType) const
{
  // Parse parent class
  const WRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    HashPropertiesRecursive(acc, uiHash, pParentType);

  // Parse properties
  WUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (WUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const WAbstractProperty* pProperty = pType->GetProperties()[i];

    if (pProperty->GetFlags().IsSet(WPropertyFlags::ReadOnly))
      continue;
    if (pProperty->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    if (pProperty->GetCategory() == WPropertyCategory::Member)
    {
      const WVariant var = acc.GetValue(pProperty->GetPropertyName());
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetCategory() == WPropertyCategory::Array || pProperty->GetCategory() == WPropertyCategory::Set)
    {
      WTempHybridArray<WVariant, 16> keys;
      acc.GetValues(pProperty->GetPropertyName(), keys);
      for (const WVariant& var : keys)
      {
        uiHash = var.ComputeHash(uiHash);
      }
    }
    else if (pProperty->GetCategory() == WPropertyCategory::Map)
    {
      WTempHybridArray<WVariant, 16> keys;
      acc.GetKeys(pProperty->GetPropertyName(), keys);
      keys.Sort([](const WVariant& a, const WVariant& b)
        { return a.Get<WString>().Compare(b.Get<WString>()) < 0; });
      for (const WVariant& key : keys)
      {
        uiHash = key.ComputeHash(uiHash);
        WVariant value = acc.GetValue(pProperty->GetPropertyName(), key);
        uiHash = value.ComputeHash(uiHash);
      }
    }
  }
}

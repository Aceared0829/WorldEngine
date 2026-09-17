#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentRoot, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Children", m_RootObjects)->AddFlags(WPropertyFlags::PointerOwner),
    W_ARRAY_MEMBER_PROPERTY("TempObjects", m_TempObjects)->AddFlags(WPropertyFlags::PointerOwner)->AddAttributes(new WTemporaryAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WDocumentRootObject::InsertSubObject(WDocumentObject* pObject, WStringView sProperty, const WVariant& index)
{
  if (sProperty.IsEmpty())
    sProperty = "Children";
  return WDocumentObject::InsertSubObject(pObject, sProperty, index);
}

void WDocumentRootObject::RemoveSubObject(WDocumentObject* pObject)
{
  return WDocumentObject::RemoveSubObject(pObject);
}

WVariant WDocumentObjectPropertyEvent::getInsertIndex() const
{
  if (m_EventType == Type::PropertyMoved)
  {
    const WIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    const WRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sProperty);
    if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
    {
      WInt32 iCurrentIndex = m_OldIndex.ConvertTo<WInt32>();
      WInt32 iNewIndex = m_NewIndex.ConvertTo<WInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return WVariant(iNewIndex);
      }
    }
  }
  return m_NewIndex;
}

WDocumentObjectManager::Storage::Storage(const WRTTI* pRootType)
  : m_RootObject(pRootType)
{
}

WDocumentObjectManager::WDocumentObjectManager(const WRTTI* pRootType)
{
  auto pStorage = W_DEFAULT_NEW(Storage, pRootType);
  pStorage->m_RootObject.m_pDocumentObjectManager = this;
  SwapStorage(pStorage);
}

WDocumentObjectManager::~WDocumentObjectManager()
{
  if (m_pObjectStorage->GetRefCount() == 1)
  {
    W_ASSERT_DEV(m_pObjectStorage->m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
  }
}

////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

WDocumentObject* WDocumentObjectManager::CreateObject(const WRTTI* pRtti, WUuid guid)
{
  W_ASSERT_DEV(pRtti != nullptr, "Unknown RTTI type");

  WDocumentObject* pObject = InternalCreateObject(pRtti);
  // In case the storage is swapped, objects should still be created in their original document manager.
  pObject->m_pDocumentObjectManager = m_pObjectStorage->m_RootObject.GetDocumentObjectManager();

  if (guid.IsValid())
    pObject->m_Guid = guid;
  else
    pObject->m_Guid = WUuid::MakeUuid();

  PatchEmbeddedClassObjectsInternal(pObject, pRtti, false);

  WDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = WDocumentObjectEvent::Type::AfterObjectCreated;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  return pObject;
}

void WDocumentObjectManager::DestroyObject(WDocumentObject* pObject)
{
  for (WDocumentObject* pChild : pObject->m_Children)
  {
    DestroyObject(pChild);
  }

  WDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = WDocumentObjectEvent::Type::BeforeObjectDestroyed;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  InternalDestroyObject(pObject);
}

void WDocumentObjectManager::DestroyAllObjects()
{
  for (auto child : m_pObjectStorage->m_RootObject.m_Children)
  {
    DestroyObject(child);
  }

  m_pObjectStorage->m_RootObject.m_Children.Clear();
  m_pObjectStorage->m_GuidToObject.Clear();
}

void WDocumentObjectManager::PatchEmbeddedClassObjects(const WDocumentObject* pObject) const
{
  // Functional should be callable from anywhere but will of course have side effects.
  const_cast<WDocumentObjectManager*>(this)->PatchEmbeddedClassObjectsInternal(
    const_cast<WDocumentObject*>(pObject), pObject->GetTypeAccessor().GetType(), true);
}

const WDocumentObject* WDocumentObjectManager::GetObject(const WUuid& guid) const
{
  const WDocumentObject* pObject = nullptr;
  if (m_pObjectStorage->m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }
  else if (guid == m_pObjectStorage->m_RootObject.GetGuid())
    return &m_pObjectStorage->m_RootObject;
  return nullptr;
}

WDocumentObject* WDocumentObjectManager::GetObject(const WUuid& guid)
{
  return const_cast<WDocumentObject*>(((const WDocumentObjectManager*)this)->GetObject(guid));
}

////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager Property Change
////////////////////////////////////////////////////////////////////////

WStatus WDocumentObjectManager::SetValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& newValue, WVariant index)
{
  W_ASSERT_DEBUG(pObject, "Object must not be null.");
  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  WVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.SetValue(sProperty, newValue, index))
  {
    return WStatus(WFmt("Set Property: The property '{0}' does not exist or value type does not match", sProperty));
  }

  WDocumentObjectPropertyEvent e;
  e.m_EventType = WDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_NewValue = newValue;
  e.m_sProperty = sProperty;
  e.m_NewIndex = index;

  // Allow a recursion depth of 2 for property setters. This allowed for two levels of side-effects on property setters.
  m_pObjectStorage->m_PropertyEvents.Broadcast(e, 2);
  return WStatus(W_SUCCESS);
}

WStatus WDocumentObjectManager::InsertValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& newValue, WVariant index)
{
  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  if (!accessor.InsertValue(sProperty, index, newValue))
  {
    if (!accessor.GetType()->FindPropertyByName(sProperty))
    {
      return WStatus(WFmt("Insert Property: The property '{0}' does not exist", sProperty));
    }
    return WStatus(WFmt("Insert Property: The property '{0}' already has the key '{1}'", sProperty, index));
  }

  WDocumentObjectPropertyEvent e;
  e.m_EventType = WDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_pObject = pObject;
  e.m_NewValue = newValue;
  e.m_NewIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return WStatus(W_SUCCESS);
}

WStatus WDocumentObjectManager::RemoveValue(WDocumentObject* pObject, WStringView sProperty, WVariant index)
{
  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  WVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.RemoveValue(sProperty, index))
  {
    return WStatus(WFmt("Remove Property: The index '{0}' in property '{1}' does not exist!", index.ConvertTo<WString>(), sProperty));
  }

  WDocumentObjectPropertyEvent e;
  e.m_EventType = WDocumentObjectPropertyEvent::Type::PropertyRemoved;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_OldIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return WStatus(W_SUCCESS);
}

WStatus WDocumentObjectManager::MoveValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& oldIndex, const WVariant& newIndex)
{
  if (!oldIndex.CanConvertTo<WInt32>() || !newIndex.CanConvertTo<WInt32>())
    return WStatus("Move Property: Invalid indices provided.");

  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  WInt32 iCount = accessor.GetCount(sProperty);
  if (iCount < 0)
    return WStatus("Move Property: Invalid property.");
  if (oldIndex.ConvertTo<WInt32>() < 0 || oldIndex.ConvertTo<WInt32>() >= iCount)
    return WStatus(WFmt("Move Property: Invalid old index '{0}'.", oldIndex.ConvertTo<WInt32>()));
  if (newIndex.ConvertTo<WInt32>() < 0 || newIndex.ConvertTo<WInt32>() > iCount)
    return WStatus(WFmt("Move Property: Invalid new index '{0}'.", newIndex.ConvertTo<WInt32>()));

  if (!accessor.MoveValue(sProperty, oldIndex, newIndex))
    return WStatus("Move Property: Move value failed.");

  {
    WDocumentObjectPropertyEvent e;
    e.m_EventType = WDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject = pObject;
    e.m_OldIndex = oldIndex;
    e.m_NewIndex = newIndex;
    e.m_sProperty = sProperty;
    e.m_NewValue = accessor.GetValue(sProperty, e.getInsertIndex());
    // NewValue can be invalid if an invalid variant in a variant array is moved
    // W_ASSERT_DEV(e.m_NewValue.IsValid(), "Value at new pos should be valid now, index missmatch?");
    m_pObjectStorage->m_PropertyEvents.Broadcast(e);
  }

  return WStatus(W_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void WDocumentObjectManager::AddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index)
{
  if (pParent == nullptr)
    pParent = &m_pObjectStorage->m_RootObject;
  if (pParent == &m_pObjectStorage->m_RootObject && sParentProperty.IsEmpty())
    sParentProperty = "Children";

  W_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an WObjectManagerBase!");
  W_ASSERT_DEV(CanAdd(pObject->GetTypeAccessor().GetType(), pParent, sParentProperty, index).Succeeded(), "Trying to execute invalid add!");

  InternalAddObject(pObject, pParent, sParentProperty, index);
}

void WDocumentObjectManager::RemoveObject(WDocumentObject* pObject)
{
  W_ASSERT_DEV(CanRemove(pObject).Succeeded(), "Trying to execute invalid remove!");
  InternalRemoveObject(pObject);
}

void WDocumentObjectManager::MoveObject(WDocumentObject* pObject, WDocumentObject* pNewParent, WStringView sParentProperty, WVariant index)
{
  W_ASSERT_DEV(CanMove(pObject, pNewParent, sParentProperty, index).Succeeded(), "Trying to execute invalid move!");

  InternalMoveObject(pNewParent, pObject, sParentProperty, index);
}


////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

WStatus WDocumentObjectManager::CanAdd(
  const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {
    const WDocumentObject* pObjectInTree = GetObject(pParent->GetGuid());
    W_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");
    if (pObjectInTree == nullptr)
      return WStatus("Parent is not part of the object manager!");

    const WIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    const WRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(sParentProperty);
    if (pProp == nullptr)
      return WStatus(WFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

    const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

    if (bIsValueType || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
    {
      return WStatus("Need to use 'InsertValue' action instead.");
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
    {
      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        if (!pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          return WStatus(WFmt("Cannot add object to the pointer property '{0}' as it does not hold ownership.", sParentProperty));

        if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
          return WStatus(WFmt("Cannot add object to the pointer property '{0}' as its type '{1}' is not derived from the property type '{2}'!",
            sParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
      else
      {
        if (pRtti != pProp->GetSpecificType())
          return WStatus(WFmt("Cannot add object to the property '{0}' as its type '{1}' does not match the property type '{2}'!", sParentProperty,
            pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
    }

    if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
    {
      WInt32 iCount = accessor.GetCount(sParentProperty);
      if (!index.CanConvertTo<WInt32>())
      {
        return WStatus(WFmt("Cannot add object to the property '{0}', the given index is an invalid WVariant (Either use '-1' to append "
                              "or a valid index).",
          sParentProperty));
      }
      WInt32 iNewIndex = index.ConvertTo<WInt32>();
      if (iNewIndex > (WInt32)iCount)
        return WStatus(WFmt(
          "Cannot add object to its new location '{0}' is out of the bounds of the parent's property range '{1}'!", iNewIndex, (WInt32)iCount));
      if (iNewIndex < 0 && iNewIndex != -1)
        return WStatus(WFmt("Cannot add object to the property '{0}', the index '{1}' is not valid (Either use '-1' to append or a valid index).",
          sParentProperty, iNewIndex));
    }
    if (pProp->GetCategory() == WPropertyCategory::Map)
    {
      if (!index.IsA<WString>())
        return WStatus(WFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
      WVariant value = accessor.GetValue(sParentProperty, index);
      if (value.IsValid() && value.IsA<WUuid>())
      {
        WUuid guid = value.Get<WUuid>();
        if (guid.IsValid())
          return WStatus(
            WFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<WString>()));
      }
    }
    else if (pProp->GetCategory() == WPropertyCategory::Member)
    {
      if (!pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        return WStatus("Embedded classes cannot be changed manually.");

      WVariant value = accessor.GetValue(sParentProperty);
      if (!value.IsA<WUuid>())
        return WStatus("Property is not a pointer and thus can't be added to.");

      if (value.Get<WUuid>().IsValid())
        return WStatus("Can't set pointer if it already has a value, need to delete value first.");
    }
  }

  return InternalCanAdd(pRtti, pParent, sParentProperty, index);
}

WStatus WDocumentObjectManager::CanRemove(const WDocumentObject* pObject) const
{
  const WDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return WStatus("Object is not part of the object manager!");

  if (pObject->GetParent())
  {
    const WAbstractProperty* pProp = pObject->GetParentPropertyType();
    W_ASSERT_DEV(pProp != nullptr, "Parent property should always be valid!");
    if (pProp->GetCategory() == WPropertyCategory::Member && !pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      return WStatus("Non pointer members can't be deleted!");
  }
  W_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

WStatus WDocumentObjectManager::CanMove(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const
{
  W_SUCCEED_OR_RETURN(CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, sParentProperty, index));

  W_SUCCEED_OR_RETURN(CanRemove(pObject));

  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return WStatus("Can't move object onto itself!");

  const WDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return WStatus("Object is not part of the object manager!");

  W_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const WDocumentObject* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return WStatus("New parent is not part of the object manager!");

    W_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const WDocumentObject* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return WStatus("Can't move object to one of its children!");

    pCurParent = pCurParent->GetParent();
  }

  const WIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
  const WRTTI* pType = accessor.GetType();

  auto* pProp = pType->FindPropertyByName(sParentProperty);

  if (pProp == nullptr)
    return WStatus(WFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

  if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
  {
    WInt32 iChildIndex = index.ConvertTo<WInt32>();
    if (iChildIndex == -1)
    {
      iChildIndex = pNewParent->GetTypeAccessor().GetCount(sParentProperty);
    }

    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      WIReflectedTypeAccessor& oldAccessor = pObject->m_pParent->GetTypeAccessor();
      WInt32 iCurrentIndex = oldAccessor.GetPropertyChildIndex(sParentProperty, pObject->GetGuid()).ConvertTo<WInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return WStatus("Can't move object onto itself!");
    }
  }
  if (pProp->GetCategory() == WPropertyCategory::Map)
  {
    if (!index.IsA<WString>())
      return WStatus(WFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
    WVariant value = accessor.GetValue(sParentProperty, index);
    if (value.IsValid() && value.IsA<WUuid>())
    {
      WUuid guid = value.Get<WUuid>();
      if (guid.IsValid())
        return WStatus(
          WFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<WString>()));
    }
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, sParentProperty, index);
}

WStatus WDocumentObjectManager::CanSelect(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "pObject must be valid");

  const WDocumentObject* pOwnObject = GetObject(pObject->GetGuid());
  if (pOwnObject == nullptr)
    return WStatus(
      WFmt("Object of type '{0}' is not part of the document and can't be selected", pObject->GetTypeAccessor().GetType()->GetTypeName()));

  return InternalCanSelect(pObject);
}


bool WDocumentObjectManager::IsUnderRootProperty(WStringView sRootProperty, const WDocumentObject* pObject) const
{
  W_ASSERT_DEBUG(m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pObject->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return sRootProperty == pObject->GetParentProperty();
}


bool WDocumentObjectManager::IsUnderRootProperty(WStringView sRootProperty, const WDocumentObject* pParent, WStringView sParentProperty) const
{
  W_ASSERT_DEBUG(pParent == nullptr || m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pParent->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return sParentProperty == sRootProperty;
  }
  return IsUnderRootProperty(sRootProperty, pParent);
}

bool WDocumentObjectManager::IsTemporary(const WDocumentObject* pObject) const
{
  return IsUnderRootProperty("TempObjects", pObject);
}

bool WDocumentObjectManager::IsTemporary(const WDocumentObject* pParent, WStringView sParentProperty) const
{
  return IsUnderRootProperty("TempObjects", pParent, sParentProperty);
}

WSharedPtr<WDocumentObjectManager::Storage> WDocumentObjectManager::SwapStorage(WSharedPtr<WDocumentObjectManager::Storage> pNewStorage)
{
  W_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pObjectStorage;

  m_StructureEventsUnsubscriber.Unsubscribe();
  m_PropertyEventsUnsubscriber.Unsubscribe();
  m_ObjectEventsUnsubscriber.Unsubscribe();

  m_pObjectStorage = pNewStorage;

  m_pObjectStorage->m_StructureEvents.AddEventHandler([this](const WDocumentObjectStructureEvent& e)
    { m_StructureEvents.Broadcast(e); }, m_StructureEventsUnsubscriber);
  m_pObjectStorage->m_PropertyEvents.AddEventHandler([this](const WDocumentObjectPropertyEvent& e)
    { m_PropertyEvents.Broadcast(e, 2); }, m_PropertyEventsUnsubscriber);
  m_pObjectStorage->m_ObjectEvents.AddEventHandler([this](const WDocumentObjectEvent& e)
    { m_ObjectEvents.Broadcast(e); }, m_ObjectEventsUnsubscriber);

  return retVal;
}

////////////////////////////////////////////////////////////////////////
// WDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void WDocumentObjectManager::InternalAddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index)
{
  WDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = WDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_sParentProperty = sParentProperty;
  e.m_NewPropertyIndex = index;

  if (e.m_NewPropertyIndex.CanConvertTo<WInt32>() && e.m_NewPropertyIndex.ConvertTo<WInt32>() == -1)
  {
    WIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pParent->InsertSubObject(pObject, sParentProperty, e.m_NewPropertyIndex);
  RecursiveAddGuids(pObject);

  e.m_EventType = WDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void WDocumentObjectManager::InternalRemoveObject(WDocumentObject* pObject)
{
  WDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = WDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;
  e.m_sParentProperty = pObject->m_sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pObject->m_pParent->RemoveSubObject(pObject);
  RecursiveRemoveGuids(pObject);

  e.m_EventType = WDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void WDocumentObjectManager::InternalMoveObject(
  WDocumentObject* pNewParent, WDocumentObject* pObject, WStringView sParentProperty, WVariant index)
{
  if (pNewParent == nullptr)
    pNewParent = &m_pObjectStorage->m_RootObject;

  WDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = WDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_sParentProperty = sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  e.m_NewPropertyIndex = index;
  if (e.m_NewPropertyIndex.CanConvertTo<WInt32>() && e.m_NewPropertyIndex.ConvertTo<WInt32>() == -1)
  {
    WIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }

  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  WVariant newIndex = e.getInsertIndex();

  pObject->m_pParent->RemoveSubObject(pObject);
  pNewParent->InsertSubObject(pObject, sParentProperty, newIndex);

  e.m_EventType = WDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  e.m_EventType = WDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void WDocumentObjectManager::RecursiveAddGuids(WDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject[pObject->m_Guid] = pObject;

  for (WUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void WDocumentObjectManager::RecursiveRemoveGuids(WDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject.Remove(pObject->m_Guid);

  for (WUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void WDocumentObjectManager::PatchEmbeddedClassObjectsInternal(WDocumentObject* pObject, const WRTTI* pType, bool addToDoc)
{
  const WRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    PatchEmbeddedClassObjectsInternal(pObject, pParent, addToDoc);

  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  const WUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (WUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const WAbstractProperty* pProperty = pType->GetProperties()[i];
    const WVariantTypeInfo* pInfo = WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProperty->GetSpecificType());

    if (pProperty->GetCategory() == WPropertyCategory::Member && pProperty->GetFlags().IsSet(WPropertyFlags::Class) && !pInfo &&
        !pProperty->GetFlags().IsSet(WPropertyFlags::Pointer))
    {
      WUuid value = accessor.GetValue(pProperty->GetPropertyName()).Get<WUuid>();
      W_ASSERT_DEV(addToDoc || !value.IsValid(), "If addToDoc is false, the current value must be invalid!");
      if (value.IsValid())
      {
        WDocumentObject* pEmbeddedObject = GetObject(value);
        if (pEmbeddedObject)
        {
          if (pEmbeddedObject->GetTypeAccessor().GetType() == pProperty->GetSpecificType())
            continue;
          else
          {
            // Type mismatch, delete old.
            InternalRemoveObject(pEmbeddedObject);
          }
        }
      }

      // Create new
      WStringBuilder sTemp;
      WConversionUtils::ToString(pObject->GetGuid(), sTemp);
      sTemp.Append("/", pProperty->GetPropertyName());
      const WUuid subObjectGuid = WUuid::MakeStableUuidFromString(sTemp);
      WDocumentObject* pEmbeddedObject = CreateObject(pProperty->GetSpecificType(), subObjectGuid);
      if (addToDoc)
      {
        InternalAddObject(pEmbeddedObject, pObject, pProperty->GetPropertyName(), WVariant());
      }
      else
      {
        pObject->InsertSubObject(pEmbeddedObject, pProperty->GetPropertyName(), WVariant());
      }
    }
  }
}


const WAbstractProperty* WDocumentObjectStructureEvent::GetProperty() const
{
  return m_pObject->GetParentPropertyType();
}

WVariant WDocumentObjectStructureEvent::getInsertIndex() const
{
  if ((m_EventType == Type::BeforeObjectMoved || m_EventType == Type::AfterObjectMoved || m_EventType == Type::AfterObjectMoved2) &&
      m_pNewParent == m_pPreviousParent)
  {
    const WIReflectedTypeAccessor& accessor = m_pPreviousParent->GetTypeAccessor();
    const WRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sParentProperty);
    if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
    {
      WInt32 iCurrentIndex = m_OldPropertyIndex.ConvertTo<WInt32>();
      WInt32 iNewIndex = m_NewPropertyIndex.ConvertTo<WInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return WVariant(iNewIndex);
      }
    }
  }
  return m_NewPropertyIndex;
}

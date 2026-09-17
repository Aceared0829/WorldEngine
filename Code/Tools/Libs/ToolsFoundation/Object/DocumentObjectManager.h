#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocumentObjectManager;
class WDocument;

// Prevent conflicts with windows.h
#ifdef GetObject
#  undef GetObject
#endif

/// Standard root object for most documents.
/// m_RootObjects stores what is in the document and m_TempObjects stores transient data used during editing which is not part of the document.
class W_TOOLSFOUNDATION_DLL WDocumentRoot : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentRoot, WReflectedClass);

  WHybridArray<WReflectedClass*, 1> m_RootObjects;
  WHybridArray<WReflectedClass*, 1> m_TempObjects;
};

/// Implementation detail of WDocumentObjectManager.
class WDocumentRootObject : public WDocumentStorageObject
{
public:
  WDocumentRootObject(const WRTTI* pRootType)
    : WDocumentStorageObject(pRootType)
  {
    m_Guid = WUuid::MakeStableUuidFromString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(WDocumentObject* pObject, WStringView sProperty, const WVariant& index) override;
  virtual void RemoveSubObject(WDocumentObject* pObject) override;
};

/// Used by WDocumentObjectManager::m_StructureEvents.
struct WDocumentObjectStructureEvent
{
  WDocumentObjectStructureEvent() = default;

  const WAbstractProperty* GetProperty() const;
  WVariant getInsertIndex() const;
  enum class Type
  {
    BeforeReset,
    AfterReset,
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
    AfterObjectMoved2,
  };

  Type m_EventType;
  const WDocument* m_pDocument = nullptr;
  const WDocumentObject* m_pObject = nullptr;
  const WDocumentObject* m_pPreviousParent = nullptr;
  const WDocumentObject* m_pNewParent = nullptr;
  WString m_sParentProperty;
  WVariant m_OldPropertyIndex;
  WVariant m_NewPropertyIndex;
};

/// Used by WDocumentObjectManager::m_PropertyEvents.
struct WDocumentObjectPropertyEvent
{
  WDocumentObjectPropertyEvent() { m_pObject = nullptr; }
  WVariant getInsertIndex() const;

  enum class Type
  {
    PropertySet,
    PropertyInserted,
    PropertyRemoved,
    PropertyMoved,
  };

  Type m_EventType;
  const WDocumentObject* m_pObject;
  WVariant m_OldValue;
  WVariant m_NewValue;
  WString m_sProperty;
  WVariant m_OldIndex;
  WVariant m_NewIndex;
};

/// Used by WDocumentObjectManager::m_ObjectEvents.
struct WDocumentObjectEvent
{
  WDocumentObjectEvent() { m_pObject = nullptr; }

  enum class Type
  {
    BeforeObjectDestroyed,
    AfterObjectCreated,
    Invalid
  };

  Type m_EventType = Type::Invalid;
  const WDocumentObject* m_pObject;
};

/// Represents to content of a document. Every document has exactly one root object under which all objects need to be parented. The default root object is WDocumentRoot.
class W_TOOLSFOUNDATION_DLL WDocumentObjectManager
{
public:
  // Storage for the object manager so it can be swapped when using multiple sub documents.
  class Storage : public WRefCounted
  {
  public:
    Storage(const WRTTI* pRootType);

    WDocument* m_pDocument = nullptr;
    WDocumentRootObject m_RootObject;

    WHashTable<WUuid, const WDocumentObject*> m_GuidToObject;

    mutable WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&> m_StructureEvents;
    mutable WCopyOnBroadcastEvent<const WDocumentObjectPropertyEvent&> m_PropertyEvents;
    WEvent<const WDocumentObjectEvent&> m_ObjectEvents;
  };

public:
  mutable WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&> m_StructureEvents;
  mutable WCopyOnBroadcastEvent<const WDocumentObjectPropertyEvent&> m_PropertyEvents;
  WEvent<const WDocumentObjectEvent&> m_ObjectEvents;

  WDocumentObjectManager(const WRTTI* pRootType = WDocumentRoot::GetStaticRTTI());
  virtual ~WDocumentObjectManager();
  void SetDocument(WDocument* pDocument) { m_pObjectStorage->m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  WDocumentObject* CreateObject(const WRTTI* pRtti, WUuid guid = WUuid());

  void DestroyObject(WDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const {};

  void PatchEmbeddedClassObjects(const WDocumentObject* pObject) const;

  const WDocumentObject* GetRootObject() const { return &m_pObjectStorage->m_RootObject; }
  WDocumentObject* GetRootObject() { return &m_pObjectStorage->m_RootObject; }
  const WDocumentObject* GetObject(const WUuid& guid) const;
  WDocumentObject* GetObject(const WUuid& guid);
  const WDocument* GetDocument() const { return m_pObjectStorage->m_pDocument; }
  WDocument* GetDocument() { return m_pObjectStorage->m_pDocument; }

  // Property Change
  WStatus SetValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& newValue, WVariant index = WVariant());
  WStatus InsertValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& newValue, WVariant index = WVariant());
  WStatus RemoveValue(WDocumentObject* pObject, WStringView sProperty, WVariant index = WVariant());
  WStatus MoveValue(WDocumentObject* pObject, WStringView sProperty, const WVariant& oldIndex, const WVariant& newIndex);

  // Structure Change
  void AddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index);
  void RemoveObject(WDocumentObject* pObject);
  void MoveObject(WDocumentObject* pObject, WDocumentObject* pNewParent, WStringView sParentProperty, WVariant index);

  // Structure Change Test
  WStatus CanAdd(const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const;
  WStatus CanRemove(const WDocumentObject* pObject) const;
  WStatus CanMove(const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const;
  WStatus CanSelect(const WDocumentObject* pObject) const;

  bool IsUnderRootProperty(WStringView sRootProperty, const WDocumentObject* pObject) const;
  bool IsUnderRootProperty(WStringView sRootProperty, const WDocumentObject* pParent, WStringView sParentProperty) const;
  bool IsTemporary(const WDocumentObject* pObject) const;
  bool IsTemporary(const WDocumentObject* pParent, WStringView sParentProperty) const;

  WSharedPtr<WDocumentObjectManager::Storage> SwapStorage(WSharedPtr<WDocumentObjectManager::Storage> pNewStorage);
  WSharedPtr<WDocumentObjectManager::Storage> GetStorage() { return m_pObjectStorage; }

private:
  virtual WDocumentObject* InternalCreateObject(const WRTTI* pRtti) { return W_DEFAULT_NEW(WDocumentStorageObject, pRtti); }
  virtual void InternalDestroyObject(WDocumentObject* pObject) { W_DEFAULT_DELETE(pObject); }

  void InternalAddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index);
  void InternalRemoveObject(WDocumentObject* pObject);
  void InternalMoveObject(WDocumentObject* pNewParent, WDocumentObject* pObject, WStringView sParentProperty, WVariant index);

  virtual WStatus InternalCanAdd(const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
  {
    return WStatus(W_SUCCESS);
  };
  virtual WStatus InternalCanRemove(const WDocumentObject* pObject) const { return WStatus(W_SUCCESS); };
  virtual WStatus InternalCanMove(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const
  {
    return WStatus(W_SUCCESS);
  };
  virtual WStatus InternalCanSelect(const WDocumentObject* pObject) const { return WStatus(W_SUCCESS); };

  void RecursiveAddGuids(WDocumentObject* pObject);
  void RecursiveRemoveGuids(WDocumentObject* pObject);
  void PatchEmbeddedClassObjectsInternal(WDocumentObject* pObject, const WRTTI* pType, bool addToDoc);

private:
  friend class WObjectAccessorBase;

  WSharedPtr<WDocumentObjectManager::Storage> m_pObjectStorage;

  WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventsUnsubscriber;
  WCopyOnBroadcastEvent<const WDocumentObjectPropertyEvent&>::Unsubscriber m_PropertyEventsUnsubscriber;
  WEvent<const WDocumentObjectEvent&>::Unsubscriber m_ObjectEventsUnsubscriber;
};

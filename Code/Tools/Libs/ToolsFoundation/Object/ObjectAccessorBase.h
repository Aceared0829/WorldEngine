#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WDocumentObject;

class W_TOOLSFOUNDATION_DLL WObjectAccessorBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WObjectAccessorBase, WReflectedClass);

public:
  virtual ~WObjectAccessorBase();
  const WDocumentObjectManager* GetObjectManager() const;

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(WStringView sDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const WDocumentObject* GetObject(const WUuid& object) = 0;
  virtual WStatus GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) = 0;
  virtual WStatus SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) = 0;
  virtual WStatus InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) = 0;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) = 0;
  virtual WStatus MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) = 0;
  virtual WStatus GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount) = 0;

  virtual WStatus AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType,
    WUuid& inout_objectGuid) = 0;
  virtual WStatus RemoveObject(const WDocumentObject* pObject) = 0;
  virtual WStatus MoveObject(const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index) = 0;

  virtual WStatus GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys) = 0;
  virtual WStatus GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values) = 0;

  /// If this accessor is a proxy accessor, transform the input parameters into those of the source accessor. The default implementation does nothing and returns this.
  /// Usually this only needs to be implemented on WObjectProxyAccessor derived accessors that modify the type, property, view etc of an object.
  /// @param ref_pObject In: proxy object, out: source object.
  /// @param ref_pType In: proxy type, out: source type.
  /// @param ref_pProp In: proxy property, out: source property.
  /// @param ref_indices In: proxy indices, out: source indices. While most of the time this will be one index, e.g. an array or map index. In case of variants that can store containers in containers this can be a chain of indices into a variant hierarchy.
  /// @return Returns the source accessor.
  virtual WObjectAccessorBase* ResolveProxy(const WDocumentObject*& ref_pObject, const WRTTI*& ref_pType, const WAbstractProperty*& ref_pProp, WDynamicArray<WVariant>& ref_indices) { return this; }
  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  WStatus GetValueByName(const WDocumentObject* pObject, WStringView sProp, WVariant& out_value, WVariant index = WVariant());
  WStatus SetValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& newValue, WVariant index = WVariant());
  WStatus InsertValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& newValue, WVariant index = WVariant());
  WStatus RemoveValueByName(const WDocumentObject* pObject, WStringView sProp, WVariant index = WVariant());
  WStatus MoveValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& oldIndex, const WVariant& newIndex);
  WStatus GetCountByName(const WDocumentObject* pObject, WStringView sProp, WInt32& out_iCount);

  WStatus AddObjectByName(const WDocumentObject* pParent, WStringView sParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid);
  WStatus MoveObjectByName(const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProp, const WVariant& index);

  WStatus GetKeysByName(const WDocumentObject* pObject, WStringView sProp, WDynamicArray<WVariant>& out_keys);
  WStatus GetValuesByName(const WDocumentObject* pObject, WStringView sProp, WDynamicArray<WVariant>& out_values);
  const WDocumentObject* GetChildObjectByName(const WDocumentObject* pObject, WStringView sProp, WVariant index);

  WStatus ClearByName(const WDocumentObject* pObject, WStringView sProp);

  const WAbstractProperty* FindPropertyByName(const WDocumentObject* pObject, WStringView sProp);

  template <typename T>
  T Get(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant());
  template <typename T>
  T GetByName(const WDocumentObject* pObject, WStringView sProp, WVariant index = WVariant());
  WInt32 GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp);
  WInt32 GetCountByName(const WDocumentObject* pObject, WStringView sProp);

  ///@}

protected:
  WObjectAccessorBase(const WDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const WDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const WDocumentObjectPropertyEvent& e);

protected:
  const WDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>

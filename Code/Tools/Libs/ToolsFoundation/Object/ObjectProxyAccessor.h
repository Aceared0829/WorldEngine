#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class WDocumentObject;

class W_TOOLSFOUNDATION_DLL WObjectProxyAccessor : public WObjectAccessorBase
{
  W_ADD_DYNAMIC_REFLECTION(WObjectProxyAccessor, WObjectAccessorBase);

public:
  WObjectProxyAccessor(WObjectAccessorBase* pSource);
  virtual ~WObjectProxyAccessor();
  WObjectAccessorBase* GetSourceAccessor() const { return m_pSource; }

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(WStringView sDisplayString) override;
  virtual void CancelTransaction() override;
  virtual void FinishTransaction() override;
  virtual void BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  virtual void CancelTemporaryCommands() override;
  virtual void FinishTemporaryCommands() override;

  ///@}
  /// \name WObjectAccessorBase overrides
  ///@{

  virtual const WDocumentObject* GetObject(const WUuid& object) override;
  virtual WStatus GetValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) override;
  virtual WStatus SetValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus InsertValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;
  virtual WStatus GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount) override;

  virtual WStatus AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType,
    WUuid& inout_objectGuid) override;
  virtual WStatus RemoveObject(const WDocumentObject* pObject) override;
  virtual WStatus MoveObject(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index) override;

  virtual WStatus GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys) override;
  virtual WStatus GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values) override;

  ///@}

protected:
  WObjectAccessorBase* m_pSource = nullptr;
};

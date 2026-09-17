#pragma once

#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

class WDocumentObject;
class WCommandHistory;

class W_TOOLSFOUNDATION_DLL WObjectCommandAccessor : public WObjectDirectAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WObjectCommandAccessor, WObjectDirectAccessor);

public:
  WObjectCommandAccessor(WCommandHistory* pHistory);

  virtual void StartTransaction(WStringView sDisplayString) override;
  virtual void CancelTransaction() override;
  virtual void FinishTransaction() override;
  virtual void BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  virtual void CancelTemporaryCommands() override;
  virtual void FinishTemporaryCommands() override;

  virtual WStatus SetValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus InsertValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;

  virtual WStatus AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType,
    WUuid& inout_objectGuid) override;
  virtual WStatus RemoveObject(const WDocumentObject* pObject) override;
  virtual WStatus MoveObject(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index) override;

protected:
  WCommandHistory* m_pHistory;
};

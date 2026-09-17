#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

/// Command accessor for visual graph documents.
///
/// Extends the base command accessor to handle visual graph-specific operations.
/// When node properties change (such as adding or removing dynamic pins), it automatically
/// disconnects and reconnects pins as needed to maintain graph consistency.
class W_TOOLSFOUNDATION_DLL WVisualGraphCommandAccessor : public WObjectCommandAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WVisualGraphCommandAccessor, WObjectCommandAccessor);

public:
  WVisualGraphCommandAccessor(WCommandHistory* pHistory);
  ~WVisualGraphCommandAccessor();

  virtual WStatus SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;

  virtual WStatus InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;

  virtual WStatus AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid) override;
  virtual WStatus RemoveObject(const WDocumentObject* pObject) override;

  bool IsNode(const WDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const;

  struct ConnectionInfo
  {
    const WDocumentObject* m_pSource = nullptr;
    const WDocumentObject* m_pTarget = nullptr;
    WString m_sSourcePin;
    WString m_sTargetPin;
  };

  WStatus DisconnectAllPins(const WDocumentObject* pObject, WDynamicArray<ConnectionInfo>& out_oldConnections);
  WStatus TryReconnectAllPins(const WDocumentObject* pObject, const WDynamicArray<ConnectionInfo>& oldConnections);
};

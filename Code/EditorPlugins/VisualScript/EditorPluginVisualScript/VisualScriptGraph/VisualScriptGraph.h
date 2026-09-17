#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

struct WVisualScriptVariable;

/// Visual graph pin for visual script nodes.
///
/// Extends the base pin with visual scripting metadata including execution flow pins, data type information,
/// and support for type deduction. Pins can represent both execution flow and data connections.
class WVisualScriptPin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptPin, WVisualGraphPin);

public:
  WVisualScriptPin(Type type, WStringView sName, const WVisualScriptNodeRegistry::PinDesc& pinDesc, const WDocumentObject* pObject, WUInt32 uiDataPinIndex, WUInt32 uiElementIndex);
  ~WVisualScriptPin();

  W_ALWAYS_INLINE bool IsExecutionPin() const { return m_pDesc->IsExecutionPin(); }
  W_ALWAYS_INLINE bool IsDataPin() const { return m_pDesc->IsDataPin(); }

  W_ALWAYS_INLINE const WRTTI* GetDataType() const { return m_pDesc->m_pDataType; }
  W_ALWAYS_INLINE WVisualScriptDataType::Enum GetScriptDataType() const { return m_pDesc->m_ScriptDataType; }
  WVisualScriptDataType::Enum GetResolvedScriptDataType() const;
  WStringView GetDataTypeName() const;
  W_ALWAYS_INLINE WUInt32 GetDataPinIndex() const { return m_uiDataPinIndex; }
  W_ALWAYS_INLINE WUInt32 GetElementIndex() const { return m_uiElementIndex; }
  W_ALWAYS_INLINE bool IsRequired() const { return m_pDesc->m_bRequired; }
  W_ALWAYS_INLINE bool HasDynamicPinProperty() const { return m_pDesc->m_sDynamicPinProperty.IsEmpty() == false; }
  W_ALWAYS_INLINE bool SplitExecution() const { return m_pDesc->m_bSplitExecution; }
  W_ALWAYS_INLINE bool ReplaceWithArray() const { return m_pDesc->m_bReplaceWithArray; }
  W_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_pDesc->m_DeductTypeFunc != nullptr; }

  W_ALWAYS_INLINE const WHashedString& GetDynamicPinProperty() const { return m_pDesc->m_sDynamicPinProperty; }
  W_ALWAYS_INLINE WVisualScriptNodeRegistry::PinDesc::DeductTypeFunc GetDeductTypeFunc() const { return m_pDesc->m_DeductTypeFunc; }

  bool CanConvertTo(const WVisualScriptPin& targetPin, bool bUseResolvedDataTypes = true) const;

private:
  const WVisualScriptNodeRegistry::PinDesc* m_pDesc = nullptr;
  WUInt32 m_uiDataPinIndex = 0;
  WUInt32 m_uiElementIndex = 0;
};

/// Object manager for visual script graphs.
///
/// Manages visual script nodes and their connections, including both execution flow and data flow.
/// Handles complex features such as type deduction, dynamic pin creation, variable management,
/// and coroutine detection. Validates connections based on script data types and execution flow rules.
class WVisualScriptNodeManager : public WVisualGraphObjectManager
{
public:
  WVisualScriptNodeManager();
  ~WVisualScriptNodeManager();

  WHashedString GetScriptBaseClass() const;
  bool IsFilteredByBaseClass(const WRTTI* pNodeType, const WVisualScriptNodeRegistry::NodeDesc& nodeDesc, const WHashedString& sBaseClass, bool bLogWarning = false) const;

  WVisualScriptDataType::Enum GetVariableType(WTempHashedString sName) const;
  WResult GetVariable(WTempHashedString sName, WVisualScriptVariable& out_variable) const;
  void GetAllVariables(WDynamicArray<WVisualScriptVariable>& out_variables) const;

  void GetInputExecutionPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const;
  void GetOutputExecutionPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const;

  void GetInputDataPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const;
  void GetOutputDataPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const;

  void GetEntryNodes(const WDocumentObject* pObject, WDynamicArray<const WDocumentObject*>& out_entryNodes) const;

  static WStringView GetNiceTypeName(const WDocumentObject* pObject);
  static WStringView GetNiceFunctionName(const WDocumentObject* pObject);

  WVisualScriptDataType::Enum GetDeductedType(const WVisualScriptPin& pin) const;
  WVisualScriptDataType::Enum GetDeductedType(const WDocumentObject* pObject) const;

  bool IsCoroutine(const WDocumentObject* pObject) const;
  bool IsLoop(const WDocumentObject* pObject) const;

  WEvent<const WDocumentObject*> m_NodeChangedEvent;

private:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual bool InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const override;
  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const override;

  void NodeEventsHandler(const WVisualGraphObjectManagerEvent& e);
  void PropertyEventsHandler(const WDocumentObjectPropertyEvent& e);

  friend class WVisualScriptPin;
  void RemoveDeductedPinType(const WVisualScriptPin& pin);
  void DeductNodeTypeAndAllPinTypes(const WDocumentObject* pObject, const WVisualGraphPin* pDisconnectedPin = nullptr);
  void UpdateCoroutine(const WDocumentObject* pTargetNode, const WVisualGraphConnection& changedConnection, bool bIsAboutToDisconnect = false);
  bool IsConnectedToCoroutine(const WDocumentObject* pEntryNode, const WVisualGraphConnection& changedConnection, bool bIsAboutToDisconnect = false) const;

  WHashTable<const WDocumentObject*, WEnum<WVisualScriptDataType>> m_ObjectToDeductedType;
  WHashTable<const WVisualScriptPin*, WEnum<WVisualScriptDataType>> m_PinToDeductedType;
  WHashSet<const WDocumentObject*> m_CoroutineObjects;

  mutable WDynamicArray<WVisualGraphNodeProperty> m_PropertyValues;
  mutable WDeque<WString> m_VariableNodeTypeNames;
};

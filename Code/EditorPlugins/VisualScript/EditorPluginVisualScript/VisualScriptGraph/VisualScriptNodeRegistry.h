#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct WVisualGraphNodeProperty;
class WVisualScriptPin;

class WVisualScriptNodeRegistry
{
  W_DECLARE_SINGLETON(WVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    WHashedString m_sName;
    WHashedString m_sDynamicPinProperty;
    const WRTTI* m_pDataType = nullptr;

    using DeductTypeFunc = WVisualScriptDataType::Enum (*)(const WVisualScriptPin& pin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    WEnum<WVisualScriptDataType> m_ScriptDataType;
    bool m_bRequired = false;
    bool m_bSplitExecution = false;
    bool m_bReplaceWithArray = false;

    W_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == WVisualScriptDataType::Invalid; }
    W_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != WVisualScriptDataType::Invalid; }

    static WColor GetColorForScriptDataType(WVisualScriptDataType::Enum dataType);
    WColor GetColor() const;
  };

  struct NodeDesc
  {
    WSmallArray<PinDesc, 4> m_InputPins;
    WSmallArray<PinDesc, 4> m_OutputPins;
    WHashedString m_sFilterByBaseClass;
    const WRTTI* m_pTargetType = nullptr;
    WSmallArray<const WAbstractProperty*, 1> m_TargetProperties;

    using DeductTypeFunc = WVisualScriptDataType::Enum (*)(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    WEnum<WVisualScriptNodeDescription::Type> m_Type;
    bool m_bImplicitExecution = true;
    bool m_bHasDynamicPins = false;

    void AddInputExecutionPin(WStringView sName, const WHashedString& sDynamicPinProperty = WHashedString());
    void AddOutputExecutionPin(WStringView sName, const WHashedString& sDynamicPinProperty = WHashedString(), bool bSplitExecution = false);

    void AddInputDataPin(WStringView sName, const WRTTI* pDataType, WVisualScriptDataType::Enum scriptDataType, bool bRequired, const WHashedString& sDynamicPinProperty = WHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr, bool bReplaceWithArray = false);
    void AddOutputDataPin(WStringView sName, const WRTTI* pDataType, WVisualScriptDataType::Enum scriptDataType, const WHashedString& sDynamicPinProperty = WHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

    W_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }
  };

  WVisualScriptNodeRegistry();
  ~WVisualScriptNodeRegistry();

  const WRTTI* GetNodeBaseType() const { return m_pBaseType; }
  const WRTTI* GetVariableSetterType() const { return m_pSetVariableType; }
  const WRTTI* GetVariableGetterType() const { return m_pGetVariableType; }
  const NodeDesc* GetNodeDescForType(const WRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  struct NodeCreationTemplate
  {
    const WRTTI* m_pType = nullptr;
    WStringView m_sTypeName;
    WHashedString m_sCategory;
    WUInt32 m_uiPropertyValuesStart;
    WUInt32 m_uiPropertyValuesCount;
  };

  const WArrayPtr<const NodeCreationTemplate> GetNodeCreationTemplates() const { return m_NodeCreationTemplates; }
  const WArrayPtr<const WVisualGraphNodeProperty> GetPropertyValues() const { return m_PropertyValues; }

  static constexpr const char* s_szTypeNamePrefix = "VisualScriptNode_";
  static constexpr WUInt32 s_uiTypeNamePrefixLength = WStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const WRTTI* pRtti, bool bForceExpose = false);

  WResult GetScriptDataType(const WRTTI* pRtti, WVisualScriptDataType::Enum& out_scriptDataType, WStringView sFunctionName = WStringView(), WStringView sArgName = WStringView());
  WVisualScriptDataType::Enum GetScriptDataType(const WAbstractProperty* pProp);

  template <typename T>
  void AddInputDataPin(WReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, WStringView sName);
  void AddInputDataPin_Any(WReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, WStringView sName, bool bRequired, bool bAddVariantProperty = false, PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

  template <typename T>
  void AddOutputDataPin(NodeDesc& ref_nodeDesc, WStringView sName);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const WRTTI* pRtti);
  void CreateFunctionCallNodeType(const WRTTI* pRtti, const WHashedString& sCategory, const WAbstractFunctionProperty* pFunction, const WScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction);
  void CreateCoroutineNodeType(const WRTTI* pRtti);
  void CreateMessageNodeTypes(const WRTTI* pRtti);
  void CreateEnumNodeTypes(const WRTTI* pRtti);

  void FillDesc(WReflectedTypeDescriptor& desc, const WRTTI* pRtti, const WColorGammaUB* pColorOverride = nullptr);
  void FillDesc(WReflectedTypeDescriptor& desc, WStringView sTypeName, const WColorGammaUB& color);

  const WRTTI* RegisterNodeType(WReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const WHashedString& sCategory);

  const WRTTI* m_pBaseType = nullptr;
  const WRTTI* m_pSetPropertyType = nullptr;
  const WRTTI* m_pGetPropertyType = nullptr;
  const WRTTI* m_pSetVariableType = nullptr;
  const WRTTI* m_pGetVariableType = nullptr;
  bool m_bBuiltinTypesCreated = false;
  WHashTable<const WRTTI*, NodeDesc> m_TypeToNodeDescs;
  WHashSet<const WRTTI*> m_ExposedTypes;
  WHashSet<const WRTTI*> m_TypesToUpdate;

  WDynamicArray<NodeCreationTemplate> m_NodeCreationTemplates;
  WDynamicArray<WVisualGraphNodeProperty> m_PropertyValues;
  WSet<WString> m_PropertyNodeTypeNames;
};

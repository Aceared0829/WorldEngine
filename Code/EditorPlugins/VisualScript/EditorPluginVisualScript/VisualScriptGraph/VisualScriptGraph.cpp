#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WVisualScriptPin::WVisualScriptPin(Type type, WStringView sName, const WVisualScriptNodeRegistry::PinDesc& pinDesc, const WDocumentObject* pObject, WUInt32 uiDataPinIndex, WUInt32 uiElementIndex)
  : WVisualGraphPin(type, sName, pinDesc.GetColor(), pObject)
  , m_pDesc(&pinDesc)
  , m_uiDataPinIndex(uiDataPinIndex)
  , m_uiElementIndex(uiElementIndex)
{
  if (pinDesc.IsExecutionPin())
  {
    m_Shape = Shape::Arrow;
  }
  else
  {
    m_Shape = (pinDesc.m_ScriptDataType == WVisualScriptDataType::Array || pinDesc.m_ScriptDataType == WVisualScriptDataType::Map) ? Shape::Rect : Shape::Circle;
  }
}

WVisualScriptPin::~WVisualScriptPin()
{
  auto pManager = static_cast<WVisualScriptNodeManager*>(const_cast<WDocumentObjectManager*>(GetParent()->GetDocumentObjectManager()));
  pManager->RemoveDeductedPinType(*this);
}

WVisualScriptDataType::Enum WVisualScriptPin::GetResolvedScriptDataType() const
{
  auto scriptDataType = GetScriptDataType();
  if (scriptDataType == WVisualScriptDataType::AnyPointer || scriptDataType == WVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const WVisualScriptNodeManager*>(GetParent()->GetDocumentObjectManager());
    return pManager->GetDeductedType(*this);
  }

  return scriptDataType;
}

WStringView WVisualScriptPin::GetDataTypeName() const
{
  WVisualScriptDataType::Enum resolvedDataType = GetResolvedScriptDataType();
  if (resolvedDataType == WVisualScriptDataType::Invalid)
  {
    return WVisualScriptDataType::GetName(GetScriptDataType());
  }

  if ((resolvedDataType == WVisualScriptDataType::TypedPointer ||
        resolvedDataType == WVisualScriptDataType::EnumValue || resolvedDataType == WVisualScriptDataType::BitflagValue) &&
      GetDataType() != nullptr)
  {
    return GetDataType()->GetTypeName();
  }

  return WVisualScriptDataType::GetName(resolvedDataType);
}

bool WVisualScriptPin::CanConvertTo(const WVisualScriptPin& targetPin, bool bUseResolvedDataTypes /*= true*/) const
{
  WVisualScriptDataType::Enum sourceScriptDataType = bUseResolvedDataTypes ? GetResolvedScriptDataType() : GetScriptDataType();
  WVisualScriptDataType::Enum targetScriptDataType = bUseResolvedDataTypes ? targetPin.GetResolvedScriptDataType() : targetPin.GetScriptDataType();

  const WRTTI* pSourceDataType = GetDataType();
  const WRTTI* pTargetDataType = targetPin.GetDataType();

  if (sourceScriptDataType == WVisualScriptDataType::TypedPointer && pSourceDataType != nullptr &&
      targetScriptDataType == WVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == WVisualScriptDataType::EnumValue && pSourceDataType != nullptr &&
      targetScriptDataType == WVisualScriptDataType::EnumValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

  if (sourceScriptDataType == WVisualScriptDataType::BitflagValue && pSourceDataType != nullptr &&
      targetScriptDataType == WVisualScriptDataType::BitflagValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

  return WVisualScriptDataType::CanConvertTo(sourceScriptDataType, targetScriptDataType);
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptNodeManager::WVisualScriptNodeManager()
{
  m_NodeEvents.AddEventHandler(WMakeDelegate(&WVisualScriptNodeManager::NodeEventsHandler, this));
  m_PropertyEvents.AddEventHandler(WMakeDelegate(&WVisualScriptNodeManager::PropertyEventsHandler, this));
}

WVisualScriptNodeManager::~WVisualScriptNodeManager() = default;

WHashedString WVisualScriptNodeManager::GetScriptBaseClass() const
{
  WHashedString sBaseClass;
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    WVariant baseClass = GetRootObject()->GetChildren()[0]->GetTypeAccessor().GetValue("BaseClass");
    if (baseClass.IsA<WString>())
    {
      sBaseClass.Assign(baseClass.Get<WString>());
    }
  }
  return sBaseClass;
}

bool WVisualScriptNodeManager::IsFilteredByBaseClass(const WRTTI* pNodeType, const WVisualScriptNodeRegistry::NodeDesc& nodeDesc, const WHashedString& sBaseClass, bool bLogWarning /*= false*/) const
{
  if (nodeDesc.m_sFilterByBaseClass.IsEmpty() == false && nodeDesc.m_sFilterByBaseClass != sBaseClass)
  {
    if (bLogWarning)
    {
      WStringView sTypeName = pNodeType->GetTypeName();
      sTypeName.TrimWordStart(WVisualScriptNodeRegistry::s_szTypeNamePrefix);

      WLog::Warning("The base class function '{}' is not a function of the currently selected base class '{}' and will be skipped", sTypeName, sBaseClass);
    }

    return true;
  }

  return false;
}


WVisualScriptDataType::Enum WVisualScriptNodeManager::GetVariableType(WTempHashedString sName) const
{
  WVisualScriptVariable variable;
  if (GetVariable(sName, variable).Succeeded())
  {
    return variable.m_TypeDecl.GetDataType();
  }

  return WVisualScriptDataType::Invalid;
}

WResult WVisualScriptNodeManager::GetVariable(WTempHashedString sName, WVisualScriptVariable& out_variable) const
{
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    WUInt32 uiNumVariables = typeAccessor.GetCount("Variables");
    for (WUInt32 i = 0; i < uiNumVariables; ++i)
    {
      WVariant variableUuid = typeAccessor.GetValue("Variables", i);
      if (variableUuid.IsA<WUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<WUuid>());
      if (pVariableObject == nullptr)
        continue;

      WVariant nameVar = pVariableObject->GetTypeAccessor().GetValue("Name");
      if (nameVar.IsA<WHashedString>() == false || nameVar.Get<WHashedString>() != sName)
        continue;

      out_variable.m_sName = nameVar.Get<WHashedString>();
      out_variable.m_TypeDecl = pVariableObject->GetTypeAccessor().GetValue("Type").Get<WVisualScriptVariableTypeDeclaration>();
      out_variable.m_DefaultValue = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

void WVisualScriptNodeManager::GetAllVariables(WDynamicArray<WVisualScriptVariable>& out_variables) const
{
  out_variables.Clear();

  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    WUInt32 uiNumVariables = typeAccessor.GetCount("Variables");
    for (WUInt32 i = 0; i < uiNumVariables; ++i)
    {
      WVariant variableUuid = typeAccessor.GetValue("Variables", i);
      if (variableUuid.IsA<WUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<WUuid>());
      if (pVariableObject == nullptr)
        continue;

      auto& variable = out_variables.ExpandAndGetRef();
      variable.m_sName = pVariableObject->GetTypeAccessor().GetValue("Name").ConvertTo<WHashedString>();
      variable.m_TypeDecl = pVariableObject->GetTypeAccessor().GetValue("Type").Get<WVisualScriptVariableTypeDeclaration>();
      variable.m_DefaultValue = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
    }
  }
}

void WVisualScriptNodeManager::GetInputExecutionPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = WStaticCast<const WVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void WVisualScriptNodeManager::GetOutputExecutionPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = WStaticCast<const WVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void WVisualScriptNodeManager::GetInputDataPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = WStaticCast<const WVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void WVisualScriptNodeManager::GetOutputDataPins(const WDocumentObject* pObject, WDynamicArray<const WVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = WStaticCast<const WVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void WVisualScriptNodeManager::GetEntryNodes(const WDocumentObject* pObject, WDynamicArray<const WDocumentObject*>& out_entryNodes) const
{
  WTempHybridArray<const WDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pObject);

  WHashSet<const WDocumentObject*> visitedNodes;
  WTempHybridArray<const WVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const WDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (WVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      out_entryNodes.PushBack(pCurrentNode);
      continue;
    }

    GetInputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        const WDocumentObject* pSourceNode = pConnection->GetSourcePin().GetParent();
        if (visitedNodes.Insert(pSourceNode))
          continue;

        nodeStack.PushBack(pSourceNode);
      }
    }
  }
}

// static
WStringView WVisualScriptNodeManager::GetNiceTypeName(const WDocumentObject* pObject)
{
  WStringView sTypeName = pObject->GetType()->GetTypeName();

  while (sTypeName.TrimWordStart(WVisualScriptNodeRegistry::s_szTypeNamePrefix) ||
         sTypeName.TrimWordStart("Builtin_"))
  {
  }

  if (const char* szAngleBracket = sTypeName.FindSubString("<"))
  {
    sTypeName = WStringView(sTypeName.GetStartPointer(), szAngleBracket);
  }

  return sTypeName;
}

WStringView WVisualScriptNodeManager::GetNiceFunctionName(const WDocumentObject* pObject)
{
  WStringView sFunctionName = pObject->GetType()->GetTypeName();

  if (const char* szSeparator = sFunctionName.FindLastSubString("::"))
  {
    sFunctionName = WStringView(szSeparator + 2, sFunctionName.GetEndPointer());
  }

  return sFunctionName;
}

WVisualScriptDataType::Enum WVisualScriptNodeManager::GetDeductedType(const WVisualScriptPin& pin) const
{
  WEnum<WVisualScriptDataType> dataType = WVisualScriptDataType::Invalid;
  m_PinToDeductedType.TryGetValue(&pin, dataType);
  return dataType;
}

WVisualScriptDataType::Enum WVisualScriptNodeManager::GetDeductedType(const WDocumentObject* pObject) const
{
  WEnum<WVisualScriptDataType> dataType = WVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.TryGetValue(pObject, dataType);
  return dataType;
}

bool WVisualScriptNodeManager::IsCoroutine(const WDocumentObject* pObject) const
{
  auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && WVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
  {
    return true;
  }

  return m_CoroutineObjects.Contains(pObject);
}

bool WVisualScriptNodeManager::IsLoop(const WDocumentObject* pObject) const
{
  auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && WVisualScriptNodeDescription::Type::IsLoop(pNodeDesc->m_Type))
  {
    return true;
  }

  return false;
}

bool WVisualScriptNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(WVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType());
}

bool WVisualScriptNodeManager::InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const
{
  auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc != nullptr && pNodeDesc->m_bHasDynamicPins)
  {
    WTempHashedString sPropNameHashed = WTempHashedString(pProp->GetPropertyName());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }

    for (auto& pinDesc : pNodeDesc->m_OutputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }
  }

  return false;
}

WStatus WVisualScriptNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  const WVisualScriptPin& pinSource = WStaticCast<const WVisualScriptPin&>(source);
  const WVisualScriptPin& pinTarget = WStaticCast<const WVisualScriptPin&>(target);

  if (pinSource.IsExecutionPin() != pinTarget.IsExecutionPin())
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget, false) == false)
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus(WFmt("The pin data types are incompatible."));
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.IsDataPin() && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return WStatus(W_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.IsExecutionPin() && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return WStatus(W_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return WStatus(W_SUCCESS);
}

void WVisualScriptNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc == nullptr)
    return;

  WTempHybridArray<WString, 16> dynamicPinNames;
  auto CreatePins = [&](const WVisualScriptNodeRegistry::PinDesc& pinDesc, WVisualGraphPin::Type type, WDynamicArray<WUniquePtr<WVisualGraphPin>>& out_pins, WUInt32& inout_dataPinIndex)
  {
    if (pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
    {
      GetDynamicPinNames(pObject, pinDesc.m_sDynamicPinProperty, pinDesc.m_sName, dynamicPinNames);
    }
    else
    {
      dynamicPinNames.Clear();
      dynamicPinNames.PushBack(pinDesc.m_sName.GetView());
    }

    for (WUInt32 i = 0; i < dynamicPinNames.GetCount(); ++i)
    {
      WUInt32 uiDataPinIndex = WInvalidIndex;
      if (pinDesc.IsDataPin())
      {
        uiDataPinIndex = inout_dataPinIndex;
        ++inout_dataPinIndex;
      }

      auto pPin = W_DEFAULT_NEW(WVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, uiDataPinIndex, i);
      out_pins.PushBack(pPin);
    }
  };

  WUInt32 uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, WVisualGraphPin::Type::Input, ref_node.m_Inputs, uiDataPinIndex);
  }

  uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, WVisualGraphPin::Type::Output, ref_node.m_Outputs, uiDataPinIndex);
  }
}

void WVisualScriptNodeManager::GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const
{
  auto pRegistry = WVisualScriptNodeRegistry::GetSingleton();
  auto propertyValues = pRegistry->GetPropertyValues();
  WHashedString sBaseClass = GetScriptBaseClass();

  for (auto& nodeTemplate : pRegistry->GetNodeCreationTemplates())
  {
    const WRTTI* pNodeType = nodeTemplate.m_pType;

    if (IsFilteredByBaseClass(pNodeType, *pRegistry->GetNodeDescForType(pNodeType), sBaseClass))
      continue;

    if (!pNodeType->GetTypeFlags().IsSet(WTypeFlags::Abstract))
    {
      auto& temp = out_templates.ExpandAndGetRef();
      temp.m_pType = pNodeType;
      temp.m_sTypeName = nodeTemplate.m_sTypeName;
      temp.m_sCategory = nodeTemplate.m_sCategory;
      temp.m_PropertyValues = propertyValues.GetSubArray(nodeTemplate.m_uiPropertyValuesStart, nodeTemplate.m_uiPropertyValuesCount);
    }
  }

  // Getter and setter templates for variables
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    static WHashedString sVariables = WMakeHashedString("Variables");
    static WHashedString sName = WMakeHashedString("Name");

    m_PropertyValues.Clear();
    m_VariableNodeTypeNames.Clear();

    WStringBuilder sNodeTypeName;

    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    const WUInt32 uiNumVariables = typeAccessor.GetCount(sVariables.GetView());

    // Pre-allocate the necessary memory for the property values since we take slices of it in the loop below,
    // thus we must not reallocate the array otherwise the slices would become invalid.
    m_PropertyValues.Reserve(uiNumVariables);

    for (WUInt32 i = 0; i < uiNumVariables; ++i)
    {
      WVariant variableUuid = typeAccessor.GetValue(sVariables.GetView(), i);
      if (variableUuid.IsA<WUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<WUuid>());
      if (pVariableObject == nullptr)
        continue;

      WVariant nameVar = pVariableObject->GetTypeAccessor().GetValue(sName.GetView());
      if (nameVar.IsA<WHashedString>() == false)
        continue;

      WHashedString sVariableName = nameVar.Get<WHashedString>();

      WUInt32 uiStart = m_PropertyValues.GetCount();
      m_PropertyValues.PushBack({sName, nameVar});

      // Setter
      {
        sNodeTypeName.Set("Set", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableSetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }

      // Getter
      {
        sNodeTypeName.Set("Get", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableGetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }
    }
  }
}

void WVisualScriptNodeManager::NodeEventsHandler(const WVisualGraphObjectManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case WVisualGraphObjectManagerEvent::Type::AfterPinsConnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent());
      UpdateCoroutine(targetPin.GetParent(), connection);
    }
    break;

    case WVisualGraphObjectManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent(), &targetPin);
      UpdateCoroutine(targetPin.GetParent(), connection, false);
    }
    break;

    case WVisualGraphObjectManagerEvent::Type::AfterNodeAdded:
    {
      DeductNodeTypeAndAllPinTypes(e.m_pObject);
    }
    break;

    case WVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved:
    {
      m_ObjectToDeductedType.Remove(e.m_pObject);
    }
    break;

    default:
      break;
  }
}

void WVisualScriptNodeManager::PropertyEventsHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject))
  {
    DeductNodeTypeAndAllPinTypes(e.m_pObject);
  }
  else if (e.m_pObject->GetType() == WGetStaticRTTI<WVisualScriptVariable>() && (e.m_sProperty == "Name" || e.m_sProperty == "Type"))
  {
    // a variable's name or type has changed, re-run type deduction
    for (auto pObject : GetRootObject()->GetChildren())
    {
      if (IsNode(pObject) == false)
        continue;

      DeductNodeTypeAndAllPinTypes(pObject);
    }

    if (e.m_sProperty == "Type")
    {
      auto typeDecl = e.m_NewValue.Get<WVisualScriptVariableTypeDeclaration>();
      WVariant defaultValue = e.m_pObject->GetTypeAccessor().GetValue("DefaultValue");
      WVisualScriptVariable::ConvertDefaultValue(defaultValue, typeDecl);

      GetDocument()->GetObjectAccessor()->SetValueByName(e.m_pObject, "DefaultValue", defaultValue).AssertSuccess();
    }
  }
}

void WVisualScriptNodeManager::RemoveDeductedPinType(const WVisualScriptPin& pin)
{
  m_PinToDeductedType.Remove(&pin);
}

void WVisualScriptNodeManager::DeductNodeTypeAndAllPinTypes(const WDocumentObject* pObject, const WVisualGraphPin* pDisconnectedPin /*= nullptr*/)
{
  auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc == nullptr || pNodeDesc->NeedsTypeDeduction() == false)
    return;

  if (pDisconnectedPin != nullptr && static_cast<const WVisualScriptPin*>(pDisconnectedPin)->NeedsTypeDeduction() == false)
    return;

  bool bNodeTypeChanged = false;
  {
    WEnum<WVisualScriptDataType> newDeductedType = pNodeDesc->m_DeductTypeFunc(pObject, static_cast<const WVisualScriptPin*>(pDisconnectedPin));
    WEnum<WVisualScriptDataType> oldDeductedType = WVisualScriptDataType::Invalid;
    m_ObjectToDeductedType.Insert(pObject, newDeductedType, &oldDeductedType);

    bNodeTypeChanged = (newDeductedType != oldDeductedType);
  }

  bool bAnyInputPinChanged = false;
  WTempHybridArray<const WVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      WEnum<WVisualScriptDataType> newDeductedType = pFunc(*pPin);
      WEnum<WVisualScriptDataType> oldDeductedType = WVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyInputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  bool bAnyOutputPinChanged = false;
  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      WEnum<WVisualScriptDataType> newDeductedType = pFunc(*pPin);
      WEnum<WVisualScriptDataType> oldDeductedType = WVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyOutputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  if (bNodeTypeChanged || bAnyInputPinChanged || bAnyOutputPinChanged)
  {
    m_NodeChangedEvent.Broadcast(pObject);
  }

  // propagate to connected nodes
  if (bAnyOutputPinChanged)
  {
    for (auto pPin : pins)
    {
      if (pPin->NeedsTypeDeduction() == false)
        continue;

      auto connections = GetConnections(*pPin);
      for (auto& connection : connections)
      {
        DeductNodeTypeAndAllPinTypes(connection->GetTargetPin().GetParent());
      }
    }
  }
}

void WVisualScriptNodeManager::UpdateCoroutine(const WDocumentObject* pTargetNode, const WVisualGraphConnection& changedConnection, bool bIsAboutToDisconnect)
{
  auto vsPin = static_cast<const WVisualScriptPin&>(changedConnection.GetTargetPin());
  if (vsPin.IsExecutionPin() == false)
    return;

  WTempHybridArray<const WDocumentObject*, 16> entryNodes;
  GetEntryNodes(pTargetNode, entryNodes);

  for (auto pEntryNode : entryNodes)
  {
    const bool bWasCoroutine = m_CoroutineObjects.Contains(pEntryNode);
    const bool bIsCoroutine = IsConnectedToCoroutine(pEntryNode, changedConnection, bIsAboutToDisconnect);

    if (bWasCoroutine != bIsCoroutine)
    {
      if (bIsCoroutine)
      {
        m_CoroutineObjects.Insert(pEntryNode);
      }
      else
      {
        m_CoroutineObjects.Remove(pEntryNode);
      }

      m_NodeChangedEvent.Broadcast(pEntryNode);
    }
  }
}

bool WVisualScriptNodeManager::IsConnectedToCoroutine(const WDocumentObject* pEntryNode, const WVisualGraphConnection& changedConnection, bool bIsAboutToDisconnect) const
{
  WTempHybridArray<const WDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryNode);

  WHashSet<const WDocumentObject*> visitedNodes;
  WTempHybridArray<const WVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const WDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (WVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
    {
      return true;
    }

    GetOutputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      if (pPin->SplitExecution())
        continue;

      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        // the connection is about to be disconnected so we ignore it here
        if (bIsAboutToDisconnect && pConnection == &changedConnection)
          continue;

        const WDocumentObject* pTargetNode = pConnection->GetTargetPin().GetParent();
        if (visitedNodes.Insert(pTargetNode))
          continue;

        nodeStack.PushBack(pTargetNode);
      }
    }
  }

  return false;
}

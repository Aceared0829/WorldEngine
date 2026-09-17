#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromNodeDataType(const WVisualScriptPin& pin)
{
  auto pObject = pin.GetParent();
  auto pManager = static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->GetDeductedType(pObject);
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromTypeProperty(const WVisualScriptPin& pin)
{
  if (auto pType = GetReflectedType(pin.GetParent()))
  {
    return WVisualScriptDataType::FromRtti(pType);
  }

  return WVisualScriptDataType::Invalid;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromExpressionInput(const WVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Inputs");
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromExpressionOutput(const WVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Outputs");
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromAllInputPins(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin)
{
  auto pManager = static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  WVisualScriptDataType::Enum deductedType = WVisualScriptDataType::Invalid;

  WTempHybridArray<const WVisualScriptPin*, 16> pins;
  pManager->GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != WVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (pPin == pDisconnectedPin)
      continue;

    WVisualScriptDataType::Enum pinDataType = WVisualScriptDataType::Invalid;
    auto connections = pManager->GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      pinDataType = static_cast<const WVisualScriptPin&>(connections[0]->GetSourcePin()).GetResolvedScriptDataType();
    }
    else
    {
      WVariant var = pObject->GetTypeAccessor().GetValue(pPin->GetName());
      pinDataType = WVisualScriptDataType::FromVariantType(var.GetType());
    }

    deductedType = WMath::Max(deductedType, pinDataType);
  }

  return deductedType;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromVariableNameProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin)
{
  auto nameVar = pObject->GetTypeAccessor().GetValue("Name");
  if (nameVar.IsA<WString>())
  {
    auto pManager = static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    return pManager->GetVariableType(WTempHashedString(nameVar.Get<WString>()));
  }

  return WVisualScriptDataType::Invalid;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<WInt64>())
  {
    return static_cast<WVisualScriptDataType::Enum>(typeVar.Get<WInt64>());
  }

  return WVisualScriptDataType::Invalid;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromPropertyProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin)
{
  if (auto pProperty = GetReflectedProperty(pObject))
  {
    return WVisualScriptDataType::FromRtti(pProperty->GetSpecificType());
  }

  return WVisualScriptDataType::Invalid;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductDummy(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin)
{
  // nothing to do here
  return WVisualScriptDataType::Float;
}

// static
const WRTTI* WVisualScriptTypeDeduction::GetReflectedType(const WDocumentObject* pObject)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<WString>() == false)
    return nullptr;

  const WString& sTypeName = typeVar.Get<WString>();
  if (sTypeName.IsEmpty())
    return nullptr;

  const WRTTI* pType = WRTTI::FindTypeByName(sTypeName);
  if (pType == nullptr && sTypeName.StartsWith("W") == false)
  {
    WStringBuilder sFullTypeName;
    sFullTypeName.Set("W", typeVar.Get<WString>());
    pType = WRTTI::FindTypeByName(sFullTypeName);
  }

  if (pType == nullptr)
  {
    WLog::Error("'{}' is not a valid type", typeVar.Get<WString>());
    return nullptr;
  }

  return pType;
}

// static
const WAbstractProperty* WVisualScriptTypeDeduction::GetReflectedProperty(const WDocumentObject* pObject)
{
  auto pType = GetReflectedType(pObject);
  if (pType == nullptr)
    return nullptr;

  auto propertyVar = pObject->GetTypeAccessor().GetValue("Property");
  if (propertyVar.IsA<WString>() == false)
    return nullptr;

  const WString& sPropertyName = propertyVar.Get<WString>();
  if (sPropertyName.IsEmpty())
    return nullptr;

  const WAbstractProperty* pProperty = pType->FindPropertyByName(propertyVar.Get<WString>());

  if (pProperty == nullptr)
  {
    WLog::Error("'{}' is not a valid property of '{}'", propertyVar.Get<WString>(), pType->GetTypeName());
    return nullptr;
  }

  return pProperty;
}

// static
WVisualScriptDataType::Enum WVisualScriptTypeDeduction::DeductFromExpressionVariable(const WVisualScriptPin& pin, WStringView sPropertyName)
{
  auto pObject = pin.GetParent();

  WVariant varList = pObject->GetTypeAccessor().GetValue(sPropertyName);
  if (varList.IsA<WVariantArray>() == false)
    return WVisualScriptDataType::Invalid;

  WVariant var = varList[pin.GetDataPinIndex()];
  if (var.IsA<WUuid>() == false)
    return WVisualScriptDataType::Invalid;

  const WDocumentObject* pVarObject = pObject->GetDocumentObjectManager()->GetObject(var.Get<WUuid>());
  if (pVarObject == nullptr)
    return WVisualScriptDataType::Invalid;

  WVariant typeVar = pVarObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<WInt64>() == false)
    return WVisualScriptDataType::Invalid;

  auto expressionDataType = static_cast<WVisualScriptExpressionDataType::Enum>(typeVar.Get<WInt64>());
  return WVisualScriptExpressionDataType::GetVisualScriptDataType(expressionDataType);
}

#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class WVisualScriptPin;

class WVisualScriptTypeDeduction
{
public:
  static WVisualScriptDataType::Enum DeductFromNodeDataType(const WVisualScriptPin& pin);
  static WVisualScriptDataType::Enum DeductFromTypeProperty(const WVisualScriptPin& pin);
  static WVisualScriptDataType::Enum DeductFromExpressionInput(const WVisualScriptPin& pin);
  static WVisualScriptDataType::Enum DeductFromExpressionOutput(const WVisualScriptPin& pin);

  static WVisualScriptDataType::Enum DeductFromAllInputPins(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);
  static WVisualScriptDataType::Enum DeductFromVariableNameProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);
  static WVisualScriptDataType::Enum DeductFromScriptDataTypeProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);
  static WVisualScriptDataType::Enum DeductFromPropertyProperty(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);
  static WVisualScriptDataType::Enum DeductDummy(const WDocumentObject* pObject, const WVisualScriptPin* pDisconnectedPin);

  static const WRTTI* GetReflectedType(const WDocumentObject* pObject);
  static const WAbstractProperty* GetReflectedProperty(const WDocumentObject* pObject);

private:
  static WVisualScriptDataType::Enum DeductFromExpressionVariable(const WVisualScriptPin& pin, WStringView sPropertyName);
};

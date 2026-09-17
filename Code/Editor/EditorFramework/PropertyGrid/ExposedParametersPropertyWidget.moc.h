#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;
struct WPhantomRttiManagerEvent;

/// Helper accessor to pretend all exposed parameters always have a value defined.
/// The exposed parameters are stored as just a sparse map. Only the elements that are overwritten from their defaults are actually stored in the component. Thus, requesting the value of an exposed parameter that has not been overwritten results in failure. To fix this, this class will automatically return the default value of an exposed parameter. This allows the tooling code to always show every exposed parameter's value independent on whether it was overwritten or remains at the default value.
class W_EDITORFRAMEWORK_DLL WExposedParameterCommandAccessor : public WObjectProxyAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WExposedParameterCommandAccessor, WObjectProxyAccessor);

public:
  WExposedParameterCommandAccessor(WObjectAccessorBase* pSource, const WAbstractProperty* pParameterProp, const WAbstractProperty* pM_pParameterSourceProp);

  virtual WStatus GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) override;
  virtual WStatus SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount) override;
  virtual WStatus GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys) override;
  virtual WStatus GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values) override;

public:
  const WExposedParameters* GetExposedParams(const WDocumentObject* pObject);
  const WExposedParameter* GetExposedParam(const WDocumentObject* pObject, const char* szParamName);
  const WRTTI* GetExposedParamsType(const WDocumentObject* pObject);
  const WRTTI* GetCommonExposedParamsType(const WArrayPtr<WPropertySelection>& items);
  bool IsExposedProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp);

public:
  const WAbstractProperty* m_pParameterProp = nullptr;
  const WAbstractProperty* m_pParameterSourceProp = nullptr;
};

/// Accessor to pretend the exposed parameters map property is an object of the generated phantom type.
/// This fake type accessor is created by taking the property name and redirecting to the exposed parameter map's element under that name. As long as no code path is looking at the actual type of the object this works with any property widget. An WQtTypeWidget constructed with the exposed parameter type and this accessor will produce a normal type widget that looks like the exposed parameter type but redirects all read / writes into the exposed parameter map property. Additionally, this class ensures the value stored in the map is converted to match the property type exactly.
class W_EDITORFRAMEWORK_DLL WExposedParametersAsTypeCommandAccessor : public WObjectProxyAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WExposedParametersAsTypeCommandAccessor, WObjectProxyAccessor);

public:
  WExposedParametersAsTypeCommandAccessor(WExposedParameterCommandAccessor* pSource);
  WExposedParameterCommandAccessor* GetSourceAccessor() const { return static_cast<WExposedParameterCommandAccessor*>(m_pSource); }

  virtual WStatus GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) override;
  virtual WStatus SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;
  virtual WStatus GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount) override;
  virtual WStatus GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys) override;
  virtual WStatus GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values) override;

  virtual WObjectAccessorBase* ResolveProxy(const WDocumentObject*& ref_pObject, const WRTTI*& ref_pType, const WAbstractProperty*& ref_pProp, WDynamicArray<WVariant>& ref_indices) override;

protected:
  WStatus GetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value);
  WStatus SetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WStatus(WVariant& subValue)>& func);

  /// Make sure that any property retrieved via this accessor matches the expected property type to make sure no invalid data is passed into one of the property widgets generated under the type widget.
  void PatchPropertyType(WVariant& ref_value, const WAbstractProperty* pProp);
};

/// Custom widget for properties annotated with the WExposedParametersAttribute attribute.
/// Technically exposed parameters are stored as an WVariantDictionary but that leaves much to be desired for usability. This class uses WExposedParameterCommandAccessor to always show all exposed parameters in the dictionary even if none were overwritten. Additionally, WExposedParametersAsTypeCommandAccessor is used to project the exposed parameters into a phantom type widget to make editing exposed parameters indistinguishable from editing a normal type object. A button can be used to switch between the two representations.
class W_EDITORFRAMEWORK_DLL WQtExposedParametersPropertyWidget : public WQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  WQtExposedParametersPropertyWidget();
  virtual ~WQtExposedParametersPropertyWidget();
  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(WUInt32 index) override;
  virtual void UpdatePropertyMetaState() override;
  virtual void GetRequiredElements(WDynamicArray<WVariant>& out_keys) const override;
  virtual void DoPrepareToDie() override;

private:
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e);
  void FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate);
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  static bool s_bRawMode;

private:
  WUniquePtr<WExposedParameterCommandAccessor> m_pProxy;
  WUniquePtr<WExposedParametersAsTypeCommandAccessor> m_pTypeProxy;
  WObjectAccessorBase* m_pSourceObjectAccessor = nullptr;
  WString m_sExposedParamProperty;
  mutable WDynamicArray<WExposedParameter> m_Parameters;
  bool m_bNeedsUpdate = false;
  bool m_bNeedsMetaDataUpdate = false;

  WQtTypeWidget* m_pTypeWidget = nullptr;
  QVBoxLayout* m_pTypeViewLayout = nullptr;
  QToolButton* m_pFixMeButton = nullptr;
  QToolButton* m_pToggleRawModeButton = nullptr;
  QAction* m_pRemoveUnusedAction = nullptr;
  QAction* m_pFixTypesAction = nullptr;
};

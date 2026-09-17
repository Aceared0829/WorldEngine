#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class WExposedParametersAttribute;
class WExposedParameterCommandAccessor;
class WExposedParametersAsTypeCommandAccessor;

/// Default state provider handling variant maps with the WExposedParametersAttribute set. Reflects the default value defined in the WExposedParameter.
class W_EDITORFRAMEWORK_DLL WExposedParametersDefaultStateProvider : public WDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);
  WExposedParametersDefaultStateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  virtual WInt32 GetRootDepth() const override;
  virtual WColorGammaUB GetBackgroundColor() const override;
  virtual WString GetStateProviderName() const override { return "Exposed Parameters"; }

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;

  virtual bool IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;

protected:
  const WDocumentObject* m_pObject = nullptr;
  const WAbstractProperty* m_pProp = nullptr;
  const WExposedParametersAttribute* m_pAttrib = nullptr;
  const WAbstractProperty* m_pParameterSourceProp = nullptr;
};

/// Default state provider handling variant maps with the WExposedParametersAttribute set that are visualized as their respective phantom type.
/// This class builds on top of WExposedParametersDefaultStateProvider and only adds the logic to redirect the phantom type + phantom property requested into the actual underlying variant map of the exposed parameters.
/// The provider is only valid if the target accessor is of type WExposedParametersAsTypeCommandAccessor.
class W_EDITORFRAMEWORK_DLL WExposedParametersAsTypeDefaultStateProvider : public WExposedParametersDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  WExposedParametersAsTypeDefaultStateProvider(WExposedParametersAsTypeCommandAccessor* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;
  virtual bool IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;

private:
  WResult GetDefaultValueInternal(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WVariant& out_DefaultValue);

private:
  WExposedParametersAsTypeCommandAccessor* m_pAccessor = nullptr;
};
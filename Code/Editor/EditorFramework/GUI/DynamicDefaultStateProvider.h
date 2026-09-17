#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class WDynamicDefaultValueAttribute;
class WPropertyPath;

/// Retrieves the dynamic default state of an object or container attributed with WDynamicDefaultValueAttribute from an asset's meta data.
class W_EDITORFRAMEWORK_DLL WDynamicDefaultStateProvider : public WDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  WDynamicDefaultStateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WDocumentObject* pClassObject, const WDocumentObject* pRootObject, const WAbstractProperty* pRootProp, WInt32 iRootDepth);

  virtual WInt32 GetRootDepth() const override;
  virtual WColorGammaUB GetBackgroundColor() const override;
  virtual WString GetStateProviderName() const override { return "Dynamic"; }

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;

private:
  const WReflectedClass* GetMetaInfo(WObjectAccessorBase* pAccessor) const;
  const WResult CreatePath(WObjectAccessorBase* pAccessor, const WReflectedClass* pMeta, WPropertyPath& propertyPath, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant());

  const WDocumentObject* m_pObject = nullptr;
  const WDocumentObject* m_pClassObject = nullptr;
  const WDocumentObject* m_pRootObject = nullptr;
  const WAbstractProperty* m_pRootProp = nullptr;
  WInt32 m_iRootDepth = 0;
  const WDynamicDefaultValueAttribute* m_pAttrib = nullptr;
  const WAbstractProperty* m_pClassSourceProp = nullptr;
  const WRTTI* m_pClassType = nullptr;
  const WAbstractProperty* m_pClassProperty = nullptr;
};

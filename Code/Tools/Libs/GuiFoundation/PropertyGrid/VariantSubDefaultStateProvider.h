#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class WVariantSubAccessor;

// Default value provider for WVariantSubAccessor.
class W_GUIFOUNDATION_DLL WVariantSubDefaultStateProvider : public WDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  WVariantSubDefaultStateProvider(WVariantSubAccessor* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  virtual WInt32 GetRootDepth() const override;
  virtual WColorGammaUB GetBackgroundColor() const override;
  virtual WString GetStateProviderName() const override { return "Variant"; }

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;
  virtual bool IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;

private:
  WResult GetDefaultValueInternal(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WVariant& out_DefaultValue);

private:
  WVariantSubAccessor* m_pAccessor = nullptr;
  const WDocumentObject* m_pObject = nullptr;
  const WAbstractProperty* m_pProp = nullptr;
  WObjectAccessorBase* m_pRootAccessor = nullptr;
};

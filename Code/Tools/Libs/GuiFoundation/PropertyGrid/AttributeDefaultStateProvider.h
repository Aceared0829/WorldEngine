#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// This is the fall back default state provider which handles the default state set via the WDefaultAttribute on the reflected type.
class W_GUIFOUNDATION_DLL WAttributeDefaultStateProvider : public WDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  virtual WInt32 GetRootDepth() const override;
  virtual WColorGammaUB GetBackgroundColor() const override;
  virtual WString GetStateProviderName() const override { return "Attribute"; }

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;
};

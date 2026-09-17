#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// Default state provider that reflects the default state defined in the prefab template.
class W_GUIFOUNDATION_DLL WPrefabDefaultStateProvider : public WDefaultStateProvider
{
public:
  static WSharedPtr<WDefaultStateProvider> CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  WPrefabDefaultStateProvider(const WUuid& rootObjectGuid, const WUuid& createFromPrefab, const WUuid& prefabSeedGuid, WInt32 iRootDepth);
  virtual WInt32 GetRootDepth() const override;
  virtual WColorGammaUB GetBackgroundColor() const override;
  virtual WString GetStateProviderName() const override { return "Prefab"; }

  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) override;

private:
  const WUuid m_RootObjectGuid;
  const WUuid m_CreateFromPrefab;
  const WUuid m_PrefabSeedGuid;
  WInt32 m_iRootDepth = 0;
};

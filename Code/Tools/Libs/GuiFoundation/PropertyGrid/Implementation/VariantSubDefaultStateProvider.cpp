#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

WSharedPtr<WDefaultStateProvider> WVariantSubDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (auto variantSubAccessor = WDynamicCast<WVariantSubAccessor*>(pAccessor))
  {
    if (variantSubAccessor->GetRootProperty() == pProp)
      return W_DEFAULT_NEW(WVariantSubDefaultStateProvider, variantSubAccessor, pObject, pProp);
  }
  return nullptr;
}

WVariantSubDefaultStateProvider::WVariantSubDefaultStateProvider(WVariantSubAccessor* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
  : m_pAccessor(pAccessor)
  , m_pObject(pObject)
  , m_pProp(pProp)
{
  m_pRootAccessor = m_pAccessor->GetSourceAccessor();
  while (auto variantSubAccessor = WDynamicCast<WVariantSubAccessor*>(m_pRootAccessor))
  {
    m_pRootAccessor = variantSubAccessor->GetSourceAccessor();
  }
}

WInt32 WVariantSubDefaultStateProvider::GetRootDepth() const
{
  // As this default provider dives into the contents of a variable it has to always be executed first as all the other providers work on property granularity.
  return 1000;
}

WColorGammaUB WVariantSubDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return WColorGammaUB(0, 0, 0, 0);
}

WVariant WVariantSubDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

WStatus WVariantSubDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  W_REPORT_FAILURE("Unreachable code");
  return WStatus(W_SUCCESS);
}

bool WVariantSubDefaultStateProvider::IsDefaultValue(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  WVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

WStatus WVariantSubDefaultStateProvider::RevertProperty(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return WStatus(WFmt("Failed to retrieve default value for variant sub tree."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

WResult WVariantSubDefaultStateProvider::GetDefaultValueInternal(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WVariant& out_DefaultValue)
{
  W_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "WVariantSubDefaultStateProvider is only valid on the object and variant property it was created on.");
  // As m_pAccessor is a view into an WVariant we first need to take the same steps into the defaultValue retrieved from the root accessor to have the same view so we can compare the same subset of both WVariants.
  out_DefaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), m_pRootAccessor, pObject, pProp);

  WTempHybridArray<WVariant, 4> path;
  W_SUCCEED_OR_RETURN(m_pAccessor->GetPath(pObject, path));
  if (index.IsValid())
    path.PushBack(index);

  for (const WVariant& step : path)
  {
    WStatus res(W_SUCCESS);
    out_DefaultValue = WVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(step, &res);
    if (res.Failed())
      return W_FAILURE;
  }
  return W_SUCCESS;
}

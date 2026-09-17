#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

WSharedPtr<WDefaultStateProvider> WExposedParametersDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (pProp)
  {
    const auto* pAttrib = pProp->GetAttributeByType<WExposedParametersAttribute>();
    if (pAttrib)
    {
      return W_DEFAULT_NEW(WExposedParametersDefaultStateProvider, pAccessor, pObject, pProp);
    }
  }
  return nullptr;
}

WExposedParametersDefaultStateProvider::WExposedParametersDefaultStateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
  : m_pObject(pObject)
  , m_pProp(pProp)
{
  W_ASSERT_DEBUG(pProp->GetCategory() == WPropertyCategory::Map, "WExposedParametersAttribute must be on a map property");
  m_pAttrib = pProp->GetAttributeByType<WExposedParametersAttribute>();
  W_ASSERT_DEBUG(m_pAttrib, "WExposedParametersDefaultStateProvider was created for a property that does not have the WExposedParametersAttribute.");
  m_pParameterSourceProp = pObject->GetType()->FindPropertyByName(m_pAttrib->GetParametersSource());
  W_ASSERT_DEBUG(
    m_pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_pAttrib->GetParametersSource(), pObject->GetType()->GetTypeName());
}

WInt32 WExposedParametersDefaultStateProvider::GetRootDepth() const
{
  return 0;
}

WColorGammaUB WExposedParametersDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return WColorGammaUB(0, 0, 0, 0);
}

WVariant WExposedParametersDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  W_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "WDefaultContainerState is only valid on the object and container it was created on.");
  WExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);
  if (index.IsValid())
  {
    if (index.IsA<WString>())
    {
      const WExposedParameter* pParam = accessor.GetExposedParam(pObject, index.Get<WString>());
      if (pParam)
      {
        return pParam->m_DefaultValue;
      }
    }
    return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
  }
  else
  {
    WVariantDictionary defaultDict;
    if (const WExposedParameters* pParams = accessor.GetExposedParams(pObject))
    {
      for (WExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    return defaultDict;
  }
}

WStatus WExposedParametersDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  W_REPORT_FAILURE("Unreachable code");
  return WStatus(W_SUCCESS);
}

bool WExposedParametersDefaultStateProvider::IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  W_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "WDefaultContainerState is only valid on the object and container it was created on.");
  WExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);

  const WVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  if (index.IsValid())
  {
    WVariant value;
    WStatus res = accessor.GetValue(pObject, pProp, value, index);
    // If the key is not valid, the exposed parameter is not overwritten and thus remains at the default value.
    return res.Failed() || def == value;
  }
  else
  {
    // We consider an exposed params map to be the default if it is empty.
    // We deliberately do not use the accessor here and go directly to the object storage as the passed in pAccessor could already be an WExposedParameterCommandAccessor in which case we wouldn't truly know if anything was overwritten.
    WVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);
    return value.Get<WVariantDictionary>().GetCount() == 0;
  }
}

WStatus WExposedParametersDefaultStateProvider::RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  if (!index.IsValid())
  {
    // We override the standard implementation here to just clear the array on revert to default. This is because the exposed params work as an override of the default behavior and we can safe space and time by simply not overriding anything.
    // The GUI will take care of pretending that the values are present with their default value.
    WDeque<WAbstractGraphDiffOperation> diff;
    auto& op = diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = WAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_uiTypeVersion = 0;
    op.m_Value = WVariantDictionary();
    WDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
    return WStatus(W_SUCCESS);
  }
  return WDefaultStateProvider::RevertProperty(superPtr, pAccessor, pObject, pProp, index);
}

///////////////////////////////////////////////////////////////////////////////

WSharedPtr<WDefaultStateProvider> WExposedParametersAsTypeDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (auto pExposedParameterCommandAccessor = WDynamicCast<WExposedParametersAsTypeCommandAccessor*>(pAccessor))
  {
    return W_DEFAULT_NEW(WExposedParametersAsTypeDefaultStateProvider, pExposedParameterCommandAccessor, pObject, pProp);
  }
  return nullptr;
}

WExposedParametersAsTypeDefaultStateProvider::WExposedParametersAsTypeDefaultStateProvider(WExposedParametersAsTypeCommandAccessor* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
  : WExposedParametersDefaultStateProvider(pAccessor->GetSourceAccessor(), pObject, pAccessor->GetSourceAccessor()->m_pParameterProp)
  , m_pAccessor(pAccessor)
{
}

WVariant WExposedParametersAsTypeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

WStatus WExposedParametersAsTypeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  W_REPORT_FAILURE("Unreachable code");
  return WStatus(W_SUCCESS);
}

bool WExposedParametersAsTypeDefaultStateProvider::IsDefaultValue(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  WVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

WStatus WExposedParametersAsTypeDefaultStateProvider::RevertProperty(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return WStatus(WFmt("Failed to retrieve default value for exposed parameter."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

WResult WExposedParametersAsTypeDefaultStateProvider::GetDefaultValueInternal(WDefaultStateProvider::SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WVariant& out_DefaultValue)
{
  // As we derive from WExposedParametersDefaultStateProvider, we first need to convert the exposed parameter type, prop, accessor into the actual underlying data structure that the base class expects before calling it.
  // * The WExposedParametersAsTypeCommandAccessor proxies the WExposedParameterCommandAccessor so the proxy source is the correct accessor.
  // * The object stays the same.
  // * The property from the exposed parameter type is replaced by the actual property that stores the exposed parameters in the real object.
  // * The index is the name of the property as that is how the parameter map is generated (keyed by parameter name).
  // With these changes made, we can rely on the base class to compute the default value.
  out_DefaultValue = WExposedParametersDefaultStateProvider::GetDefaultValue(superPtr, m_pAccessor->GetSourceAccessor(), pObject, m_pAccessor->GetSourceAccessor()->m_pParameterProp, pProp->GetPropertyName());

  WStatus res(W_SUCCESS);
  // We now have the value of the exposed parameter. If this is a container, we need to dive into the index. If index is invalid, this is a no-op.
  out_DefaultValue = WVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(index, &res);
  if (res.Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

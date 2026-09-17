#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static WSharedPtr<WDefaultStateProvider> g_pAttributeDefaultStateProvider = W_DEFAULT_NEW(WAttributeDefaultStateProvider);
WSharedPtr<WDefaultStateProvider> WAttributeDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  // One global instance handles all. No need to create a new instance per request as no state need to be tracked.
  return g_pAttributeDefaultStateProvider;
}

WInt32 WAttributeDefaultStateProvider::GetRootDepth() const
{
  return -1;
}

WColorGammaUB WAttributeDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return WColorGammaUB(0, 0, 0, 0);
}

WVariant WAttributeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  if (!pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && pProp->GetFlags().IsSet(WPropertyFlags::Class) && pProp->GetCategory() == WPropertyCategory::Member && !WReflectionUtils::IsValueType(pProp))
  {
    // An embedded class that is not a value type can never change its value.
    WVariant value;
    pAccessor->GetValue(pObject, pProp, value).LogFailure();
    return value;
  }
  return WReflectionUtils::GetDefaultValue(pProp, index);
}

WStatus WAttributeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  auto RemoveObject = [&](const WUuid& object)
  {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = object;
    op.m_Operation = WAbstractGraphDiffOperation::Op::NodeRemoved;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pAccessor->GetObject(object)->GetType()->GetTypeName();
  };

  auto SetProperty = [&](const WVariant& newValue)
  {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = WAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_Value = newValue;
  };

  WVariant currentValue;
  pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      const auto& objectGuid = currentValue.Get<WUuid>();
      if (objectGuid.IsValid())
      {
        RemoveObject(objectGuid);
        SetProperty(WUuid());
      }
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      const auto& currentArray = currentValue.Get<WVariantArray>();
      for (WInt32 i = (WInt32)currentArray.GetCount() - 1; i >= 0; i--)
      {
        const auto& objectGuid = currentArray[i].Get<WUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(WVariantArray());
    }
    break;
    case WPropertyCategory::Map:
    {
      const auto& currentArray = currentValue.Get<WVariantDictionary>();
      for (auto val : currentArray)
      {
        const auto& objectGuid = val.Value().Get<WUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(WVariantDictionary());
    }
    break;
    default:
      W_REPORT_FAILURE("Unreachable code");
      break;
  }
  return WStatus(W_SUCCESS);
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetCheckRule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WObjectAccessorBase* WAssetCheckContext::GetObjectAccessor() const
{
  return m_pDocument ? m_pDocument->GetObjectAccessor() : nullptr;
}

void WAssetCheckContext::ReportIssue(WAssetCheckSeverity::Enum severity, WStringView sMessage, const WDocumentObject* pObject /*= nullptr*/, bool bFixed /*= false*/)
{
  WAssetCheckNote& note = m_pNotes->ExpandAndGetRef();
  note.m_Severity = severity;
  note.m_sMessage = sMessage;
  note.m_bFixed = bFixed;

  if (pObject != nullptr)
  {
    note.m_ObjectGuid = pObject->GetGuid();
  }

  if (bFixed)
  {
    ++m_uiFixCount;
  }
}

WString WAssetCheckContext::GetObjectDisplayName(const WDocumentObject* pObject)
{
  if (pObject == nullptr)
    return WString();

  const WVariant name = pObject->GetTypeAccessor().GetValue("Name");
  if (name.IsValid() && name.CanConvertTo<WString>())
  {
    const WString sName = name.ConvertTo<WString>();
    if (!sName.IsEmpty())
      return sName;
  }

  return pObject->GetType()->GetTypeName();
}

void WAssetCheckRule::CheckDocument(WAssetCheckContext& ref_ctx)
{
  const WDocumentObject* pRoot = ref_ctx.GetDocument()->GetObjectManager()->GetRootObject();
  VisitObjectRecursive(ref_ctx, pRoot);
}

void WAssetCheckRule::VisitObjectRecursive(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject)
{
  CheckObject(ref_ctx, pObject);

  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType() != nullptr &&
        pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    VisitObjectRecursive(ref_ctx, pChild);
  }
}

void WAssetCheckRule::CheckObject(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject)
{
  WHybridArray<const WAbstractProperty*, 32> properties;
  pObject->GetType()->GetAllProperties(properties);

  for (const WAbstractProperty* pProp : properties)
  {
    if (pProp->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    CheckProperty(ref_ctx, pObject, pProp);
  }
}

void WAssetCheckRule::CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
}

WResult WAssetCheckRule::GetPropertyValues(WObjectAccessorBase* pAcc, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_indices, WDynamicArray<WVariant>& out_values)
{
  out_indices.Clear();
  out_values.Clear();

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      WVariant value;
      if (pAcc->GetValue(pObject, pProp, value).Failed())
        return W_FAILURE;

      out_indices.PushBack(WVariant());
      out_values.PushBack(value);
      return W_SUCCESS;
    }

    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      if (pAcc->GetValues(pObject, pProp, out_values).Failed())
        return W_FAILURE;

      out_indices.Reserve(out_values.GetCount());
      for (WUInt32 i = 0; i < out_values.GetCount(); ++i)
        out_indices.PushBack(i);
      return W_SUCCESS;
    }

    case WPropertyCategory::Map:
    {
      if (pAcc->GetKeys(pObject, pProp, out_indices).Failed())
        return W_FAILURE;

      out_values.Reserve(out_indices.GetCount());
      for (const WVariant& key : out_indices)
      {
        WVariant value;
        if (pAcc->GetValue(pObject, pProp, value, key).Failed())
          return W_FAILURE;

        out_values.PushBack(value);
      }
      return W_SUCCESS;
    }

    default:
      return W_FAILURE;
  }
}

void WAssetCheckRule::CreateRules(WDynamicArray<WAssetCheckRule*>& out_rules)
{
  WRTTI::ForEachDerivedType<WAssetCheckRule>(
    [&](const WRTTI* pRtti)
    {
      out_rules.PushBack(pRtti->GetAllocator()->Allocate<WAssetCheckRule>());
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  out_rules.Sort([](WAssetCheckRule* lhs, WAssetCheckRule* rhs) -> bool
    {
      return lhs->GetDisplayName().Compare_NoCase(rhs->GetDisplayName()) < 0; //
    });
}

void WAssetCheckRule::DestroyRules(WDynamicArray<WAssetCheckRule*>& ref_rules)
{
  for (WAssetCheckRule* pRule : ref_rules)
  {
    pRule->GetDynamicRTTI()->GetAllocator()->Deallocate(pRule);
  }

  ref_rules.Clear();
}

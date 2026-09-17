#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCheckRules.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WUnknownTagsAssetCheckRule, 1, WRTTIDefaultAllocator<WUnknownTagsAssetCheckRule>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRequiredPropertyAssetCheckRule, 1, WRTTIDefaultAllocator<WRequiredPropertyAssetCheckRule>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEmptyGameObjectAssetCheckRule, 1, WRTTIDefaultAllocator<WEmptyGameObjectAssetCheckRule>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStringView WUnknownTagsAssetCheckRule::GetDescription() const
{
  return "Detects tags used on objects that are not registered in the project's tag configuration. "
         "Auto-fix removes these tags.";
}

void WUnknownTagsAssetCheckRule::CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  // A tag-set property is a Set with the tag-set widget attribute.
  if (pProp->GetCategory() != WPropertyCategory::Set)
    return;
  if (pProp->GetAttributeByType<WTagSetWidgetAttribute>() == nullptr)
    return;

  WObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();

  WHybridArray<WVariant, 16> indices;
  WHybridArray<WVariant, 16> values;
  if (GetPropertyValues(pAcc, pObject, pProp, indices, values).Failed())
    return;

  // Iterate back-to-front so that removals keep the remaining indices valid.
  for (WUInt32 i = values.GetCount(); i-- > 0;)
  {
    if (!values[i].CanConvertTo<WString>())
      continue;

    const WString sTag = values[i].ConvertTo<WString>();
    if (WToolsTagRegistry::IsTagKnown(sTag))
      continue;

    const WString sObj = WAssetCheckContext::GetObjectDisplayName(pObject);
    WStringBuilder sMsg;

    if (ref_ctx.IsAutoFixAllowed())
    {
      if (pAcc->RemoveValue(pObject, pProp, indices[i]).Succeeded())
      {
        sMsg.SetFormat("Removed unknown tag '{}' from '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
        ref_ctx.ReportIssue(WAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ true);
      }
      else
      {
        sMsg.SetFormat("Failed to remove unknown tag '{}' from '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
        ref_ctx.ReportIssue(WAssetCheckSeverity::Error, sMsg, pObject, /*bFixed*/ false);
      }
    }
    else
    {
      sMsg.SetFormat("Unknown tag '{}' on '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
      ref_ctx.ReportIssue(WAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ false);
    }
  }
}

WStringView WRequiredPropertyAssetCheckRule::GetDescription() const
{
  return "Detects properties marked with WRequiredAttribute that are empty, or, for game object / "
         "component reference properties, that reference an object which does not exist.";
}

namespace
{
  bool IsEmptyOrInvalidRequiredValue(WObjectAccessorBase* pAcc, const WAbstractProperty* pProp, const WVariant& value)
  {
    if (!value.IsValid() || !value.CanConvertTo<WString>())
      return true;

    const WString sValue = value.ConvertTo<WString>();
    if (sValue.IsEmpty())
      return true;

    // Game object / component reference properties store a stringified WUuid; make sure it resolves.
    if (pProp->GetAttributeByType<WGameObjectReferenceAttribute>() != nullptr)
    {
      if (!WConversionUtils::IsStringUuid(sValue))
        return true;

      const WUuid guid = WConversionUtils::ConvertStringToUuid(sValue);
      if (pAcc->GetObject(guid) == nullptr)
        return true;
    }

    return false;
  }

} // namespace

void WRequiredPropertyAssetCheckRule::CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (pProp->GetAttributeByType<WRequiredAttribute>() == nullptr)
    return;

  WObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();

  WHybridArray<WVariant, 16> indices;
  WHybridArray<WVariant, 16> values;
  if (GetPropertyValues(pAcc, pObject, pProp, indices, values).Failed())
    return;

  const WString sObj = WAssetCheckContext::GetObjectDisplayName(pObject);
  WStringBuilder sMsg;

  // An empty container has no element to inspect but still fails the requirement.
  if (values.IsEmpty())
  {
    sMsg.SetFormat("Required property '{}' on '{}' is empty.", pProp->GetPropertyName(), sObj);
    ref_ctx.ReportIssue(WAssetCheckSeverity::Error, sMsg, pObject);
    return;
  }

  for (const WVariant& value : values)
  {
    if (!IsEmptyOrInvalidRequiredValue(pAcc, pProp, value))
      continue;

    sMsg.SetFormat("Required property '{}' on '{}' is empty.", pProp->GetPropertyName(), sObj);
    ref_ctx.ReportIssue(WAssetCheckSeverity::Error, sMsg, pObject);
  }
}

WStringView WEmptyGameObjectAssetCheckRule::GetDescription() const
{
  return "Detects game objects that have no name, no components and no child objects, and thus have "
         "no effect on the scene. Auto-fix removes them.";
}

bool WEmptyGameObjectAssetCheckRule::AppliesToDocumentType(WStringView sDocumentTypeName) const
{
  return sDocumentTypeName == "Scene" || sDocumentTypeName == "Prefab" || sDocumentTypeName == "Layer";
}

void WEmptyGameObjectAssetCheckRule::CheckDocument(WAssetCheckContext& ref_ctx)
{
  m_EmptyObjects.Clear();

  // The default traversal (via CheckObject below) only collects candidates; objects are removed
  // afterwards so that removal doesn't invalidate a parent frame's ongoing child iteration.
  WAssetCheckRule::CheckDocument(ref_ctx);

  WObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();
  WStringBuilder sMsg;

  for (const WDocumentObject* pObject : m_EmptyObjects)
  {
    const WString sObj = WAssetCheckContext::GetObjectDisplayName(pObject);

    if (ref_ctx.IsAutoFixAllowed())
    {
      if (pAcc->RemoveObject(pObject).Succeeded())
      {
        sMsg.SetFormat("Removed empty game object '{}' (no name, no components, no children).", sObj);
        ref_ctx.ReportIssue(WAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ true);
      }
      else
      {
        sMsg.SetFormat("Failed to remove empty game object '{}'.", sObj);
        ref_ctx.ReportIssue(WAssetCheckSeverity::Error, sMsg, pObject, /*bFixed*/ false);
      }
    }
    else
    {
      sMsg.SetFormat("Game object '{}' has no name, no components and no children.", sObj);
      ref_ctx.ReportIssue(WAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ false);
    }
  }

  m_EmptyObjects.Clear();
}

void WEmptyGameObjectAssetCheckRule::CheckObject(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject)
{
  if (!pObject->GetType()->IsDerivedFrom<WGameObject>())
    return;

  bool bHasChildOrComponent = false;
  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentProperty() == "Children" || pChild->GetParentProperty() == "Components")
    {
      bHasChildOrComponent = true;
      break;
    }
  }

  if (bHasChildOrComponent)
    return;

  WVariant name;
  if (ref_ctx.GetObjectAccessor()->GetValueByName(pObject, "Name", name).Succeeded() && name.CanConvertTo<WString>() &&
      !name.ConvertTo<WString>().IsEmpty())
    return;

  m_EmptyObjects.PushBack(pObject);
}

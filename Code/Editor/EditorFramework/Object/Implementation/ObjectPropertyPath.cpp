#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

WStatus WObjectPropertyPath::CreatePath(const WObjectPropertyPathContext& context, const WPropertyReference& prop,
  WStringBuilder& ref_sObjectSearchSequence, WStringBuilder& ref_sComponentType, WStringBuilder& ref_sPropertyPath)
{
  W_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const WRTTI* pObjType = WGetStaticRTTI<WGameObject>();

  const WAbstractProperty* pName = pObjType->FindPropertyByName("Name");
  const WDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return WStatus(W_FAILURE);

  {
    // Build property part of the path from the next parent node / component.
    pObject = FindParentNodeComponent(pObject);
    if (!pObject)
      return WStatus("No parent node or component found.");
    WObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    WStatus res = CreatePropertyPath(context2, prop, ref_sPropertyPath);
    if (res.Failed())
      return res;
  }

  {
    // Component part
    ref_sComponentType.Clear();
    if (pObject->GetType()->IsDerivedFrom(WGetStaticRTTI<WComponent>()))
    {
      ref_sComponentType = pObject->GetType()->GetTypeName();
      pObject = pObject->GetParent();
    }
  }

  // Node path
  while (pObject != context.m_pContextObject)
  {
    if (pObject == nullptr)
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return WStatus("Property is not under the given context object, no path exists.");
    }

    if (pObject->GetType() == WGetStaticRTTI<WGameObject>())
    {
      WString sName = context.m_pAccessor->Get<WString>(pObject, pName);
      if (!sName.IsEmpty())
      {
        if (!ref_sObjectSearchSequence.IsEmpty())
          ref_sObjectSearchSequence.Prepend("/");
        ref_sObjectSearchSequence.Prepend(sName);
      }
    }
    else
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return WStatus(WFmt("Only WGameObject objects should be found in the hierarchy, found '{0}' instead.", pObject->GetType()->GetTypeName()));
    }

    pObject = pObject->GetParent();
  }
  return WStatus(W_SUCCESS);
}

WStatus WObjectPropertyPath::CreatePropertyPath(
  const WObjectPropertyPathContext& context, const WPropertyReference& prop, WStringBuilder& out_sPropertyPath)
{
  W_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const WDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return WStatus(W_FAILURE);

  out_sPropertyPath.Clear();
  WStatus res = PrependProperty(pObject, prop.m_pProperty, prop.m_Index, out_sPropertyPath);
  if (res.Failed())
    return res;

  while (pObject != context.m_pContextObject)
  {
    WStatus result = PrependProperty(pObject->GetParent(), pObject->GetParentPropertyType(), pObject->GetPropertyIndex(), out_sPropertyPath);
    if (result.Failed())
      return result;

    pObject = pObject->GetParent();
  }
  return WStatus(W_SUCCESS);
}

void WObjectPropertyPath::AppendSubIndices(WStringBuilder& ref_sPropertyPath, WArrayPtr<WVariant> indices)
{
  for (WUInt32 i = 0; i < indices.GetCount(); ++i)
  {
    ref_sPropertyPath.AppendFormat("[{0}]", indices[i]);
  }
}

WStatus WObjectPropertyPath::ResolvePath(const WObjectPropertyPathContext& context, WDynamicArray<WPropertyReference>& ref_keys,
  const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath)
{
  W_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  ref_keys.Clear();
  const WDocumentObject* pContext = context.m_pContextObject;
  WDocumentObjectVisitor visitor(context.m_pAccessor->GetObjectManager(), "Children", context.m_sRootProperty);
  WTempHybridArray<const WDocumentObject*, 8> input;
  input.PushBack(pContext);
  WTempHybridArray<const WDocumentObject*, 8> output;

  // Find objects that match the search path
  WStringBuilder sObjectSearchSequence = szObjectSearchSequence;
  WTempHybridArray<WStringView, 4> names;
  sObjectSearchSequence.Split(false, names, "/");
  for (const WStringView& sName : names)
  {
    for (const WDocumentObject* pObj : input)
    {
      visitor.Visit(pObj, false, [&output, &sName](const WDocumentObject* pObject) -> bool
        {
          const auto& sObjectName = pObject->GetTypeAccessor().GetValue("Name").Get<WString>();
          if (sObjectName == sName)
          {
            output.PushBack(pObject);
            return false;
          }
          return true; //
        });
    }
    input.Clear();
    input.Swap(output);
  }

  if (input.IsEmpty())
    return WStatus(WFmt("ObjectSearchSequence: '{}' could not be resolved", szObjectSearchSequence));

  // Test found objects for component
  for (const WDocumentObject* pObject : input)
  {
    // Could also be the root object in which case we found nothing.
    if (pObject->GetType() == WGetStaticRTTI<WGameObject>())
    {
      if (WStringUtils::IsNullOrEmpty(szComponentType))
      {
        // We are animating the game object directly
        output.PushBack(pObject);
      }
      else
      {
        const WInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
        for (WInt32 i = 0; i < iComponents; i++)
        {
          WVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
          auto pChild = context.m_pAccessor->GetObjectManager()->GetObject(value.Get<WUuid>());
          if (pChild->GetType()->GetTypeName() == szComponentType)
          {
            output.PushBack(pChild);
            continue; // #TODO: break on found component?
          }
        }
      }
    }
  }
  input.Clear();
  input.Swap(output);

  if (input.IsEmpty())
    return WStatus(WFmt("Component '{}' not found on the search path '{}'", szComponentType, szObjectSearchSequence));

  WStatus lastError = WResult(W_FAILURE);
  // Test found objects / components for property
  for (const WDocumentObject* pObject : input)
  {
    WObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    WPropertyReference key;
    WStatus res = ResolvePropertyPath(context2, szPropertyPath, key);
    if (res.Succeeded())
    {
      ref_keys.PushBack(key);
    }

    if (lastError.Failed())
    {
      lastError = res;
    }
  }
  return lastError;
}

WStatus WObjectPropertyPath::ResolvePropertyPath(
  const WObjectPropertyPathContext& context, const char* szPropertyPath, WPropertyReference& out_key)
{
  W_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && szPropertyPath != nullptr, "All context fields must be valid.");
  const WDocumentObject* pObject = context.m_pContextObject;
  WStringBuilder sPath = szPropertyPath;
  WTempHybridArray<WStringView, 3> parts;
  sPath.Split(false, parts, "/");
  for (WUInt32 i = 0; i < parts.GetCount(); i++)
  {
    WStringBuilder sPart = parts[i];
    WTempHybridArray<WStringBuilder, 2> parts2;
    sPart.Split(false, parts2, "[", "]");
    if (parts2.GetCount() == 0 || parts2.GetCount() > 2)
    {
      return WStatus(WFmt("Malformed property path part: {0}", sPart));
    }
    const WAbstractProperty* pProperty = pObject->GetType()->FindPropertyByName(parts2[0]);
    if (!pProperty)
      return WStatus(WFmt("Property not found: {0}", parts2[0]));
    WVariant index;
    if (parts2.GetCount() == 2)
    {
      WInt32 iIndex = 0;
      if (WConversionUtils::StringToInt(parts2[1], iIndex).Succeeded())
      {
        index = iIndex; // Array index
      }
      else
      {
        index = parts2[1].GetData(); // Map index
      }
    }

    WVariant value;
    WStatus res(W_SUCCESS);

    if (const WExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<WExposedParametersAttribute>())
    {
      const WAbstractProperty* pParameterSourceProp = pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
      W_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(),
        pObject->GetType()->GetTypeName());
      WExposedParameterCommandAccessor proxy(context.m_pAccessor, pProperty, pParameterSourceProp);
      res = proxy.GetValue(pObject, pProperty, value, index);
    }
    else
    {
      res = context.m_pAccessor->GetValue(pObject, pProperty, value, index);
    }

    if (res.Failed())
      return res;

    if (i == parts.GetCount() - 1)
    {
      out_key.m_Object = pObject->GetGuid();
      out_key.m_pProperty = pProperty;
      out_key.m_Index = index;
      return WStatus(W_SUCCESS);
    }
    else
    {
      if (value.IsA<WUuid>())
      {
        WUuid id = value.Get<WUuid>();
        pObject = context.m_pAccessor->GetObjectManager()->GetObject(id);
      }
      else
      {
        return WStatus(WFmt("Property '{0}' of type '{1}' is not an object and can't be traversed further.", pProperty->GetPropertyName(),
          pProperty->GetSpecificType()->GetTypeName()));
      }
    }
  }
  return WStatus(W_FAILURE);
}

WStatus WObjectPropertyPath::PrependProperty(
  const WDocumentObject* pObject, const WAbstractProperty* pProperty, WVariant index, WStringBuilder& out_sPropertyPath)
{
  switch (pProperty->GetCategory())
  {
    case WPropertyCategory::Enum::Member:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      out_sPropertyPath.Prepend(pProperty->GetPropertyName());
      return WStatus(W_SUCCESS);
    }
    case WPropertyCategory::Enum::Array:
    case WPropertyCategory::Enum::Map:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      if (index.IsValid())
        out_sPropertyPath.PrependFormat("{0}[{1}]", pProperty->GetPropertyName(), index);
      else
        out_sPropertyPath.PrependFormat("{0}", pProperty->GetPropertyName());
      return WStatus(W_SUCCESS);
    }
    default:
      return WStatus(WFmt(
        "The property '{0}' of category '{1}' which is not supported in property paths", pProperty->GetPropertyName(), pProperty->GetCategory()));
  }
}

const WDocumentObject* WObjectPropertyPath::FindParentNodeComponent(const WDocumentObject* pObject)
{
  const WRTTI* pObjType = WGetStaticRTTI<WGameObject>();
  const WRTTI* pCompType = WGetStaticRTTI<WComponent>();
  const WDocumentObject* pObj = pObject;
  while (pObj != nullptr)
  {
    if (pObj->GetType() == pObjType)
    {
      return pObj;
    }
    else if (pObj->GetType()->IsDerivedFrom(pCompType))
    {
      return pObj;
    }
    pObj = pObj->GetParent();
  }
  return nullptr;
}

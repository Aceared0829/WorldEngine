#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

WSharedPtr<WDefaultStateProvider> WDynamicDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (pProp)
  {
    auto* pAttrib = pProp->GetAttributeByType<WDynamicDefaultValueAttribute>();
    if (pAttrib && !WStringUtils::IsNullOrEmpty(pAttrib->GetClassProperty()))
    {
      return W_DEFAULT_NEW(WDynamicDefaultStateProvider, pAccessor, pObject, pObject, pObject, pProp, 0);
    }
  }

  WInt32 iRootDepth = 0;
  if (pProp)
    iRootDepth += 1;

  const WDocumentObject* pCurrentObject = pObject;
  while (pCurrentObject)
  {
    const WAbstractProperty* pParentProp = pCurrentObject->GetParentPropertyType();
    if (!pParentProp)
      return nullptr;

    const auto* pAttrib = pParentProp->GetAttributeByType<WDynamicDefaultValueAttribute>();
    if (pAttrib)
    {
      iRootDepth += 1;
      return W_DEFAULT_NEW(WDynamicDefaultStateProvider, pAccessor, pObject, pCurrentObject, pCurrentObject->GetParent(), pParentProp, iRootDepth);
    }
    iRootDepth += 2;
    pCurrentObject = pCurrentObject->GetParent();
  }
  return nullptr;
}

WDynamicDefaultStateProvider::WDynamicDefaultStateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WDocumentObject* pClassObject, const WDocumentObject* pRootObject, const WAbstractProperty* pRootProp, WInt32 iRootDepth)
  : m_pObject(pObject)
  , m_pClassObject(pClassObject)
  , m_pRootObject(pRootObject)
  , m_pRootProp(pRootProp)
  , m_iRootDepth(iRootDepth)
{
  m_pAttrib = m_pRootProp->GetAttributeByType<WDynamicDefaultValueAttribute>();
  W_ASSERT_DEBUG(m_pAttrib, "WDynamicDefaultStateProvider was created for a property that does not have the WDynamicDefaultValueAttribute.");

  m_pClassType = WRTTI::FindTypeByName(m_pAttrib->GetClassType());
  W_ASSERT_DEBUG(m_pClassType, "The dynamic meta data class type '{0}' does not exist", m_pAttrib->GetClassType());

  m_pClassSourceProp = m_pRootObject->GetType()->FindPropertyByName(m_pAttrib->GetClassSource());
  W_ASSERT_DEBUG(m_pClassSourceProp, "The dynamic meta data class source '{0}' does not exist on type '{1}'", m_pAttrib->GetClassSource(), m_pRootObject->GetType()->GetTypeName());

  const bool bHasProperty = !WStringUtils::IsNullOrEmpty(m_pAttrib->GetClassProperty());
  if (!bHasProperty)
  {
    W_ASSERT_DEBUG(m_pRootProp->GetCategory() == WPropertyCategory::Member, "WDynamicDefaultValueAttribute must be on a member property if no ClassProperty is given.");
  }
  else
  {
    m_pClassProperty = m_pClassType->FindPropertyByName(m_pAttrib->GetClassProperty());

    W_ASSERT_DEBUG(m_pClassProperty, "The dynamic meta data class type '{0}' does not have a property named '{1}'", m_pAttrib->GetClassType(), m_pAttrib->GetClassProperty());
  }
}

WInt32 WDynamicDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

WColorGammaUB WDynamicDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return WColorGammaUB(0, 0, 0, 0);
}

WVariant WDynamicDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);

  if (const WReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    WPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp, index).Failed())
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    WVariant defaultValue;
    WResult res = propertyPath.ReadProperty(const_cast<WReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), [&](void* pLeaf, const WRTTI& type, const WAbstractProperty* pNativeProp, const WVariant& index)
      {
      W_ASSERT_DEBUG(pProp->GetCategory() == pNativeProp->GetCategory(), "While properties don't need to match exactly, they need to be of the same category and type.");

      switch (pNativeProp->GetCategory())
      {
        case WPropertyCategory::Member:
          defaultValue = WReflectionUtils::GetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pNativeProp), pLeaf);
          break;
        case WPropertyCategory::Array:
        {
          WVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const WVariantArray& currentArray = currentValue.Get<WVariantArray>();

          auto* pArrayProp = static_cast<const WAbstractArrayProperty*>(pNativeProp);
          if (!index.IsValid())
          {
            WVariantArray varArray;
            varArray.SetCount(pArrayProp->GetCount(pLeaf));
            for (WUInt32 i = 0; i < pArrayProp->GetCount(pLeaf); i++)
            {
              if (bIsValueType)
              {
                varArray[i] = WReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, i);
              }
              else
              {
                // We don't have any guid on the native object. Thus we just match the count basically and fill everything we can't find on our current object with 'nullptr', i.e. an invalid guid.
                if (i < currentArray.GetCount())
                {
                  varArray[i] = currentArray[i];
                }
                else
                {
                  varArray[i] = WUuid();
                }
              }
            }
            defaultValue = std::move(varArray);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = WReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, index.ConvertTo<WInt32>());
            }
            else
            {
              WUInt32 iIndex = index.ConvertTo<WUInt32>();
              if (iIndex < currentArray.GetCount())
              {
                defaultValue = currentArray[iIndex];
              }
              else
              {
                defaultValue = WUuid();
              }
            }
          }
        }
        break;
        case WPropertyCategory::Map:
        {
          auto* pMapProp = static_cast<const WAbstractMapProperty*>(pNativeProp);

          WVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const WVariantDictionary& currentDict = currentValue.Get<WVariantDictionary>();

          if (!index.IsValid())
          {
            WTempHybridArray<WString, 16> keys;
            pMapProp->GetKeys(pLeaf, keys);

            WVariantDictionary varDict;
            for (auto& key : keys)
            {
              if (bIsValueType)
              {
                varDict.Insert(key, WReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, key));
              }
              else
              {
                if (auto* pValue = currentDict.GetValue(key))
                {
                  varDict.Insert(key, *pValue);
                }
                else
                {
                  varDict.Insert(key, WUuid());
                }
              }
            }
            defaultValue = std::move(varDict);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = WReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, index.Get<WString>());
            }
            else
            {
              if (auto* pValue = currentDict.GetValue(index.Get<WString>()))
              {
                defaultValue = *pValue;
              }
              else
              {
                defaultValue = WUuid();
              }
            }
          }
        }
        break;
        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      } });

    if (res.Succeeded())
    {
      if (!DoesVariantMatchProperty(defaultValue, pProp, index))
      {
        WLog::Error("Default value '{}' does not match property '{}' at index '{}'", defaultValue, pProp->GetPropertyName(), index);
      }
      else
      {
        return defaultValue;
      }
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
}

const WReflectedClass* WDynamicDefaultStateProvider::GetMetaInfo(WObjectAccessorBase* pAccessor) const
{
  WVariant value;
  if (pAccessor->GetValue(m_pRootObject, m_pClassSourceProp, value).Succeeded())
  {
    if (value.IsA<WString>())
    {
      const auto& sValue = value.Get<WString>();
      if (const auto asset = WAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo(m_pClassType);
      }
    }
  }

  return nullptr;
}

const WResult WDynamicDefaultStateProvider::CreatePath(WObjectAccessorBase* pAccessor, const WReflectedClass* pMeta, WPropertyPath& propertyPath, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WObjectPropertyPathContext pathContext = {m_pClassProperty ? m_pRootObject : m_pClassObject, pAccessor, "Children"};

  WPropertyReference ref;
  ref.m_Object = pObject->GetGuid();
  ref.m_pProperty = pProp;
  ref.m_Index = index;

  WStringBuilder sPropPath;
  WObjectPropertyPath::CreatePropertyPath(pathContext, ref, sPropPath).LogFailure();
  if (m_pClassProperty)
  {
    sPropPath.ReplaceFirst(m_pRootProp->GetPropertyName(), m_pAttrib->GetClassProperty());
  }

  return propertyPath.InitializeFromPath(*pMeta->GetDynamicRTTI(), sPropPath);
}

WStatus WDynamicDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  if (const WReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    WPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp).Failed())
    {
      return WStatus(WFmt("Failed to find root object in object graph"));
    }

    WAbstractObjectGraph prefabSubGraph;
    WAbstractObjectNode* pPrefabSubRoot = nullptr;
    {
      // Create a graph of the native object, skipping all other properties except for the container in question.
      WRttiConverterContext context;
      WString sRootPropertyName = pProp->GetPropertyName();
      // If we are dealing with an attributed container and pObject is its parent, then the root container property name can differ between the meta info and the target object so we have to rename it later to make the two graphs match.
      if (m_pClassProperty && pObject == m_pRootObject)
      {
        sRootPropertyName = m_pAttrib->GetClassProperty();
      }

      void* pNativeRootObject = nullptr;
      WRttiConverterWriter rttiConverter(&prefabSubGraph, &context, [&](const void* pObject, const WAbstractProperty* pCurrentProp)
        {
        if (pNativeRootObject == pObject && pCurrentProp->GetPropertyName() != sRootPropertyName)
          return false;
        return true; });

      auto WriteObject = [&](void* pLeafObject, const WRTTI& leafType, const WAbstractProperty* pLeafProp, const WVariant& index)
      {
        pNativeRootObject = pLeafObject;
        context.RegisterObject(pObject->GetGuid(), &leafType, pLeafObject);
        pPrefabSubRoot = rttiConverter.AddObjectToGraph(&leafType, pLeafObject);
        pPrefabSubRoot->RenameProperty(sRootPropertyName, pProp->GetPropertyName());
      };

      WVariant defaultValue;
      WResult res = propertyPath.ReadProperty(const_cast<WReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), WriteObject);
      if (res.Failed())
      {
        return WStatus(WFmt("Failed to find root object in object graph"));
      }
    }

    // Create graph from current object with only the container to be reverted present.
    WAbstractObjectGraph instanceSubGraph;
    WAbstractObjectNode* pInstanceSubRoot = nullptr;
    {
      WDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const WDocumentObject* pObject, const WAbstractProperty* pProp)
        {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;
        return true; });
      pInstanceSubRoot = writer.AddObjectToGraph(pObject);
    }

    // Make the native graph match the guids of pObject graph.
    pPrefabSubRoot->SetType(pInstanceSubRoot->GetType());
    prefabSubGraph.ReMapNodeGuidsToMatchGraph(pPrefabSubRoot, instanceSubGraph, pInstanceSubRoot);
    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);
    return WStatus(W_SUCCESS);
  }
  return superPtr[0]->CreateRevertContainerDiff(superPtr.GetSubArray(1), pAccessor, pObject, pProp, out_diff);
}

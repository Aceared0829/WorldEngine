#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

WSharedPtr<WDefaultStateProvider> WPrefabDefaultStateProvider::CreateProvider(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  const auto* pMetaData = pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.Borrow();
  WInt32 iRootDepth = 0;
  WUuid rootObjectGuid = WPrefabUtils::GetPrefabRoot(pObject, *pMetaData, &iRootDepth);
  // The root depth is taken x2 because GetPrefabRoot counts the number of parent objects while WDefaultStateProvider expects to count the properties as well.
  iRootDepth *= 2;
  // If we construct this from a property scope, the root is an additional hop away as GetPrefabRoot counts from the parent object.
  if (pProp)
    iRootDepth += 1;

  if (rootObjectGuid.IsValid())
  {
    auto pMeta = pMetaData->BeginReadMetaData(rootObjectGuid);
    W_SCOPE_EXIT(pMetaData->EndReadMetaData(););
    WUuid objectPrefabGuid = pObject->GetGuid();
    objectPrefabGuid.RevertCombinationWithSeed(pMeta->m_PrefabSeedGuid);
    const WAbstractObjectGraph* pGraph = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);
    if (pGraph)
    {
      if (pGraph->GetNode(objectPrefabGuid) != nullptr)
      {
        // The object was found in the prefab, we can thus use its prefab counterpart to provide a default state.
        return W_DEFAULT_NEW(WPrefabDefaultStateProvider, rootObjectGuid, pMeta->m_CreateFromPrefab, pMeta->m_PrefabSeedGuid, iRootDepth);
      }
    }
  }
  return nullptr;
}

WPrefabDefaultStateProvider::WPrefabDefaultStateProvider(const WUuid& rootObjectGuid, const WUuid& createFromPrefab, const WUuid& prefabSeedGuid, WInt32 iRootDepth)
  : m_RootObjectGuid(rootObjectGuid)
  , m_CreateFromPrefab(createFromPrefab)
  , m_PrefabSeedGuid(prefabSeedGuid)
  , m_iRootDepth(iRootDepth)
{
}

WInt32 WPrefabDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

WColorGammaUB WPrefabDefaultStateProvider::GetBackgroundColor() const
{
  return WColorScheme::DarkUI(WColorScheme::Blue).WithAlpha(0.25f);
}

WVariant WPrefabDefaultStateProvider::GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);

  const WAbstractObjectGraph* pGraph = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  WUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    bool bValueFound = true;
    WVariant defaultValue = WPrefabUtils::GetDefaultValue(*pGraph, objectPrefabGuid, pProp->GetPropertyName(), index, &bValueFound);
    if (!bValueFound)
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags) && defaultValue.IsA<WString>())
    {
      WInt64 iValue = 0;
      if (WReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), defaultValue.Get<WString>(), iValue))
      {
        defaultValue = iValue;
      }
      else
      {
        defaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
      }
    }
    else if (!bIsValueType)
    {
      // For object references we need to reverse the object GUID mapping from prefab -> instance.
      switch (pProp->GetCategory())
      {
        case WPropertyCategory::Member:
        {
          WUuid& targetGuid = defaultValue.GetWritable<WUuid>();
          targetGuid.CombineWithSeed(m_PrefabSeedGuid);
        }
        break;
        case WPropertyCategory::Array:
        case WPropertyCategory::Set:
        {
          if (index.IsValid())
          {
            WUuid& targetGuid = defaultValue.GetWritable<WUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            WVariantArray& defaultValueArray = defaultValue.GetWritable<WVariantArray>();
            for (WVariant& value : defaultValueArray)
            {
              WUuid& targetGuid = value.GetWritable<WUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        case WPropertyCategory::Map:
        {
          if (index.IsValid())
          {
            WUuid& targetGuid = defaultValue.GetWritable<WUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            WVariantDictionary& defaultValueDict = defaultValue.GetWritable<WVariantDictionary>();
            for (auto it : defaultValueDict)
            {
              WUuid& targetGuid = it.Value().GetWritable<WUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        default:
          break;
      }
    }

    if (defaultValue.IsValid())
    {
      if (defaultValue.IsString() && pProp->GetAttributeByType<WGameObjectReferenceAttribute>())
      {
        // While pretty expensive this restores the default state of game object references which are stored as strings.
        WStringView sValue = defaultValue.GetType() == WVariantType::StringView ? defaultValue.Get<WStringView>() : WStringView(defaultValue.Get<WString>().GetData());
        if (WConversionUtils::IsStringUuid(sValue))
        {
          WUuid guid = WConversionUtils::ConvertStringToUuid(sValue);
          guid.CombineWithSeed(m_PrefabSeedGuid);
          WStringBuilder sTemp;
          defaultValue = WConversionUtils::ToString(guid, sTemp).GetData();
        }
      }

      return defaultValue;
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp);
}

WStatus WPrefabDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff)
{
  WVariant defaultValue = GetDefaultValue(superPtr, pAccessor, pObject, pProp);
  WVariant currentValue;
  W_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, currentValue));

  const WAbstractObjectGraph* pGraph = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  WUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    // We create a sub-graph of only the parent node in both re-mapped prefab as well as from the actually object. We limit the graph to only the container property.
    auto pNode = pGraph->GetNode(objectPrefabGuid);
    WAbstractObjectGraph prefabSubGraph;
    pGraph->Clone(prefabSubGraph, pNode, [pRootNode = pNode, pRootProp = pProp](const WAbstractObjectNode* pNode, const WAbstractObjectNode::Property* pProp)
      {
        if (pNode == pRootNode && pProp->m_sPropertyName != pRootProp->GetPropertyName())
          return false;

        return true; //
      });

    prefabSubGraph.ReMapNodeGuids(m_PrefabSeedGuid);

    WAbstractObjectGraph instanceSubGraph;
    WDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const WDocumentObject* pObject, const WAbstractProperty* pProp)
      {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;

        return true; //
      });

    writer.AddObjectToGraph(pObject);

    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);

    return WStatus(W_SUCCESS);
  }

  return WStatus(WFmt("The object was not found in the base prefab graph."));
}

#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPrefabResource, 1, WRTTIDefaultAllocator<WPrefabResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WPrefabResource);
// clang-format on

WPrefabResource::WPrefabResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

void WPrefabResource::InstantiatePrefab(WWorld& ref_world, const WTransform& rootTransform, WPrefabInstantiationOptions options, const WArrayMap<WHashedString, WVariant>* pExposedParamValues)
{
  if (GetLoadingState() != WResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    WTempHybridArray<WGameObject*, 8> createdRootObjects;
    WTempHybridArray<WGameObject*, 8> createdChildObjects;

    if (options.m_pCreatedRootObjectsOut == nullptr)
    {
      options.m_pCreatedRootObjectsOut = &createdRootObjects;
    }

    if (options.m_pCreatedChildObjectsOut == nullptr)
    {
      options.m_pCreatedChildObjectsOut = &createdChildObjects;
    }

    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);

    W_ASSERT_DEBUG(options.m_pCreatedRootObjectsOut != options.m_pCreatedChildObjectsOut, "These pointers must point to different arrays, otherwise applying exposed properties doesn't work correctly.");

    // It is ok to move static objects through exposed parameter, so we disable the error message here.
    const bool bReportErrorWhenStaticObjectMoves = ref_world.ReportErrorWhenStaticObjectMoves();
    ref_world.SetReportErrorWhenStaticObjectMoves(false);

    ApplyExposedParameterValues(pExposedParamValues, *options.m_pCreatedChildObjectsOut, *options.m_pCreatedRootObjectsOut);

    // Restore the original error reporting state
    ref_world.SetReportErrorWhenStaticObjectMoves(bReportErrorWhenStaticObjectMoves);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);
  }
}

WPrefabResource::InstantiateResult WPrefabResource::InstantiatePrefab(const WPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, WWorld& ref_world, const WTransform& rootTransform, WPrefabInstantiationOptions options, const WArrayMap<WHashedString, WVariant>* pExposedParamValues /*= nullptr*/)
{
  WResourceLock<WPrefabResource> pPrefab(hPrefab, bBlockTillLoaded ? WResourceAcquireMode::BlockTillLoaded_NeverFail : WResourceAcquireMode::AllowLoadingFallback_NeverFail);

  switch (pPrefab.GetAcquireResult())
  {
    case WResourceAcquireResult::Final:
      pPrefab->InstantiatePrefab(ref_world, rootTransform, options, pExposedParamValues);
      return InstantiateResult::Success;

    case WResourceAcquireResult::LoadingFallback:
      return InstantiateResult::NotYetLoaded;

    default:
      return InstantiateResult::Error;
  }
}

void WPrefabResource::ApplyExposedParameterValues(const WArrayMap<WHashedString, WVariant>* pExposedParamValues, const WDynamicArray<WGameObject*>& createdChildObjects, const WDynamicArray<WGameObject*>& createdRootObjects) const
{
  const WUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (WUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const WHashedString& name = pExposedParamValues->GetKey(i);
    const WUInt64 uiNameHash = name.GetHash();

    for (WUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      WGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.SetValue(pTarget, pExposedParamValues->GetValue(i));
        }
        else
        {
          for (WComponent* pComp : pTarget->GetComponents())
          {
            const WRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead
            // atm if the same component type is attached multiple times, they will all get the value applied
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
            {
              ppd.m_CachedPropertyPath.SetValue(pComp, pExposedParamValues->GetValue(i));
            }
          }
        }
      }

      // Allow to bind multiple properties to the same exposed parameter name
      // Therefore, do not break here, but continue iterating
    }
  }
}

WResourceLoadDesc WPrefabResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  if (WhatToUnload == WResource::Unload::AllQualityLevels)
  {
    m_WorldReader.ClearAndCompact();
  }

  return res;
}

WResourceLoadDesc WPrefabResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WPrefabResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WStreamReader& s = *Stream;

  // the standard file reader writes the absolute file path into the stream
  WString sAbsFilePath;
  s >> sAbsFilePath;

  WAssetFileHeader assetHeader;
  assetHeader.Read(s).IgnoreResult();

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  W_ASSERT_DEV(WStringUtils::IsEqualN(szSceneTag, "[WEBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!WStringUtils::IsEqualN(szSceneTag, "[WEBinaryScene]", 16))
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s).IgnoreResult();

  if (assetHeader.GetFileVersion() >= 4)
  {
    WUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (WUInt32 i = 0; i < uiExposedParams; ++i)
    {
      auto& ppd = m_PrefabParamDescs[i];

      W_ASSERT_DEV(assetHeader.GetFileVersion() >= 6, "Old resource version not supported anymore");
      ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.InitializeFromPath(*WGetStaticRTTI<WGameObject>(), ppd.m_sProperty).IgnoreResult();
        }
        else
        {
          if (const WRTTI* pRtti = WRTTI::FindTypeByNameHash(ppd.m_sComponentType.GetHash()))
          {
            ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty).IgnoreResult();
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const WExposedPrefabParameterDesc& lhs, const WExposedPrefabParameterDesc& rhs) -> bool
      { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_WorldReader.GetHeapMemoryUsage() + sizeof(this);
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WPrefabResource, WPrefabResourceDescriptor)
{
  W_IGNORE_UNUSED(descriptor);

  WResourceLoadDesc desc;
  desc.m_State = WResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

WUInt32 WPrefabResource::FindFirstParamWithName(WUInt64 uiNameHash) const
{
  WUInt32 lb = 0;
  WUInt32 ub = m_PrefabParamDescs.GetCount();

  while (lb < ub)
  {
    const WUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_PrefabParamDescs[middle].m_sExposeName.GetHash() < uiNameHash)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  return lb;
}

void WExposedPrefabParameterDesc::Save(WStreamWriter& inout_stream) const
{
  WUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  inout_stream << m_sExposeName;
  inout_stream << comb;
  inout_stream << m_sComponentType;
  inout_stream << m_sProperty;
}

void WExposedPrefabParameterDesc::Load(WStreamReader& inout_stream)
{
  WUInt32 comb = 0;

  inout_stream >> m_sExposeName;
  inout_stream >> comb;
  inout_stream >> m_sComponentType;
  inout_stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

W_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabResource);

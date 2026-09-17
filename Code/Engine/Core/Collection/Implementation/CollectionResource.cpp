#include <Core/CorePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/AssetFileHeader.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCollectionResource, 1, WRTTIDefaultAllocator<WCollectionResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WCollectionResource);

WCollectionResource::WCollectionResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

// UnloadData() already makes sure to call UnregisterNames();
WCollectionResource::~WCollectionResource() = default;

bool WCollectionResource::PreloadResources(WUInt32 uiNumResourcesToPreload)
{
  W_LOCK(m_PreloadMutex);
  W_PROFILE_SCOPE("Inject Resources to Preload");

  if (m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount())
  {
    // All resources have already been queued so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return false;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  const WUInt32 remainingResources = m_Collection.m_Resources.GetCount() - m_PreloadedResources.GetCount();
  const WUInt32 end = WMath::Min(remainingResources, uiNumResourcesToPreload) + m_PreloadedResources.GetCount();
  for (WUInt32 i = m_PreloadedResources.GetCount(); i < end; ++i)
  {
    const WCollectionEntry& e = m_Collection.m_Resources[i];
    WTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const WRTTI* pRtti = WResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = WResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        WLog::Warning("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register the resource type with the WResourceManager?", e.m_sAssetTypeName, WArgSensitive(e.m_sResourceID, "ResourceID"));
      }
    }
    else
    {
      WLog::Error("Asset '{}' had an empty asset type name. Cannot pre-load it.", WArgSensitive(e.m_sResourceID, "ResourceID"));
    }

    m_PreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      WResourceManager::PreloadResource(hTypeless);
    }
  }

  return m_PreloadedResources.GetCount() < m_Collection.m_Resources.GetCount();
}

bool WCollectionResource::IsLoadingFinished(float* out_pProgress) const
{
  W_LOCK(m_PreloadMutex);

  WUInt64 loadedWeight = 0;
  WUInt64 totalWeight = 0;

  WUInt32 uiPoked = 0;

  for (WUInt32 i = 0; i < m_PreloadedResources.GetCount(); i++)
  {
    const WTypelessResourceHandle& hResource = m_PreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const WCollectionEntry& entry = m_Collection.m_Resources[i];
    WUInt64 thisWeight = WMath::Max(entry.m_uiFileSize, 1ull); // if file sizes are not specified, we weight by 1
    WResourceState state = WResourceManager::GetLoadingState(hResource);

    if (state == WResourceState::Loaded || state == WResourceState::LoadedResourceMissing)
    {
      loadedWeight += thisWeight;
    }
    else if (state != WResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
    else
    {
      if (uiPoked < 3)
      {
        // there's a bug or race condition somewhere when unloading resources, which means resources that should be queued
        // for preloading don't get preloaded and then the entire preloading system gets stuck
        // to prevent this, we'll make sure that the next few unloaded resources do get requeued for preload

        ++uiPoked;
        WResourceManager::PreloadResource(hResource);
      }
    }
  }

  if (out_pProgress != nullptr)
  {
    const float maxLoadedFraction = m_Collection.m_Resources.GetCount() == 0 ? 1.f : (float)m_PreloadedResources.GetCount() / m_Collection.m_Resources.GetCount();
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_pProgress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight) * maxLoadedFraction;
    }
    else
    {
      *out_pProgress = maxLoadedFraction;
    }
  }

  if (totalWeight == 0 || totalWeight == loadedWeight)
  {
    return true;
  }

  return false;
}


const WCollectionResourceDescriptor& WCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WCollectionResource, WCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WCollectionResource::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  {
    UnregisterNames();
    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    // W_LOCK(m_preloadMutex);
    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

WResourceLoadDesc WCollectionResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WCollectionResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Collection.Load(*Stream);

  res.m_State = WResourceState::Loaded;
  return res;
}

void WCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  W_LOCK(m_PreloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<WUInt32>(m_PreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
}


void WCollectionResource::RegisterNames()
{
  if (m_bRegistered)
    return;

  m_bRegistered = true;

  W_LOCK(WResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      WResourceManager::RegisterNamedResource(entry.m_sOptionalNiceLookupName, entry.m_sResourceID);
    }
  }
}


void WCollectionResource::UnregisterNames()
{
  if (!m_bRegistered)
    return;

  m_bRegistered = false;

  W_LOCK(WResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      WResourceManager::UnregisterNamedResource(entry.m_sOptionalNiceLookupName);
    }
  }
}

void WCollectionResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;
  const WUInt8 uiIdentifier = 0xC0;
  const WUInt32 uiNumResources = m_Resources.GetCount();

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << uiNumResources;

  for (WUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream << m_Resources[i].m_sAssetTypeName;
    inout_stream << m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream << m_Resources[i].m_sResourceID;
    inout_stream << m_Resources[i].m_uiFileSize;
  }
}

void WCollectionResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;
  WUInt8 uiIdentifier = 0;
  WUInt32 uiNumResources = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    WUInt16 uiNumResourcesShort;
    inout_stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    inout_stream >> uiNumResources;
  }

  W_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid WCollectionResourceDescriptor");
  W_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (WUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream >> m_Resources[i].m_sAssetTypeName;
    inout_stream >> m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      inout_stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



W_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);

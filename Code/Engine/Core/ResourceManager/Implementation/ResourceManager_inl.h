#pragma once

#include <Foundation/Logging/Log.h>

template <typename ResourceType>
W_FORCE_INLINE ResourceType* WResourceManager::GetResource(WStringView sResourceID, bool bIsReloadable)
{
  return static_cast<ResourceType*>(GetResource(WGetStaticRTTI<ResourceType>(), sResourceID, bIsReloadable));
}

template <typename ResourceType>
W_FORCE_INLINE WTypedResourceHandle<ResourceType> WResourceManager::LoadResource(WStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  W_LOCK(s_ResourceMutex);
  return WTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
}

template <typename ResourceType>
WTypedResourceHandle<ResourceType> WResourceManager::LoadResource(WStringView sResourceID, WTypedResourceHandle<ResourceType> hLoadingFallback)
{
  WTypedResourceHandle<ResourceType> hResource;
  {
    // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
    W_LOCK(s_ResourceMutex);
    hResource = WTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
  }

  if (hLoadingFallback.IsValid())
  {
    hResource.m_pResource->SetLoadingFallbackResource(hLoadingFallback);
  }

  return hResource;
}

template <typename ResourceType>
WTypedResourceHandle<ResourceType> WResourceManager::GetExistingResource(WStringView sResourceID)
{
  WResource* pResource = nullptr;

  const WTempHashedString sResourceHash(sResourceID);

  W_LOCK(s_ResourceMutex);

  const WRTTI* pRtti = FindResourceTypeOverride(WGetStaticRTTI<ResourceType>(), sResourceID);

  if (GetLoadedResources()[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return WTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return WTypedResourceHandle<ResourceType>();
}

template <typename ResourceType, typename DescriptorType>
WTypedResourceHandle<ResourceType> WResourceManager::CreateResource(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription)
{
  return CreateResourceInternal<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription, false);
}

template <typename ResourceType, typename DescriptorType>
WTypedResourceHandle<ResourceType> WResourceManager::CreateResourceInternal(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription, bool bAllowGetFallback)
{
  static_assert(std::is_rvalue_reference<DescriptorType&&>::value, "Please std::move the descriptor into this function");

  WTypedResourceHandle<ResourceType> hResource;
  ResourceType* pResource = nullptr;
  W_LOG_BLOCK("WResourceManager::CreateResource", sResourceID);
  {
    W_LOCK(s_ResourceMutex);
    // In this locked scope, we decide whether we are creating the resource or waiting for it to be created by someone else.
    if (bAllowGetFallback)
      hResource = GetExistingResource<ResourceType>(sResourceID);

    if (!hResource.IsValid())
    {
      hResource = WTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, false));
      pResource = BeginAcquireResource(hResource, WResourceAcquireMode::PointerOnly);
      pResource->SetResourceDescription(sResourceDescription);
      pResource->m_Flags.Add(WResourceFlags::IsCreatedResource);

      W_ASSERT_DEV(pResource->GetLoadingState() == WResourceState::Unloaded, "CreateResource was called on a resource that is already created");
    }
  }

  if (!pResource)
  {
    // If we didn't acquire the resource yet, someone else already did so we just wait for them to finish creation.
    pResource = BeginAcquireResource(hResource, WResourceAcquireMode::BlockTillLoaded);
    EndAcquireResource(pResource);
    return hResource;
  }


  // If this does not compile, you either passed in the wrong descriptor type for the given resource type
  // or you forgot to std::move the descriptor when calling CreateResource
  auto localDescriptor = std::move(descriptor);
  WResourceLoadDesc ld = pResource->CreateResource(std::move(localDescriptor));

  {
    W_LOCK(s_ResourceMutex);
    pResource->VerifyAfterCreateResource(ld);
    W_ASSERT_DEV(pResource->GetLoadingState() != WResourceState::Unloaded, "CreateResource did not set the loading state properly.");
  }

  EndAcquireResource(pResource);
  return hResource;
}

template <typename ResourceType, typename DescriptorType>
WTypedResourceHandle<ResourceType>
WResourceManager::GetOrCreateResource(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription)
{
  return CreateResourceInternal<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription, true);
}

W_FORCE_INLINE WResource* WResourceManager::BeginAcquireResourcePointer(const WRTTI* pType, const WTypelessResourceHandle& hResource)
{
  W_IGNORE_UNUSED(pType);
  W_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  WResource* pResource = (WResource*)hResource.m_pResource;

  W_ASSERT_DEBUG(pResource->GetDynamicRTTI()->IsDerivedFrom(pType),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    pType->GetTypeName());

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
ResourceType* WResourceManager::BeginAcquireResource(const WTypedResourceHandle<ResourceType>& hResource, WResourceAcquireMode mode,
  const WTypedResourceHandle<ResourceType>& hFallbackResource, WResourceAcquireResult* out_pAcquireResult /*= nullptr*/)
{
  W_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WResource* pCurrentlyUpdatingContent = WResource::GetCurrentlyUpdatingContent();
  if (pCurrentlyUpdatingContent != nullptr)
  {
    W_LOCK(s_ResourceMutex);
    W_ASSERT_DEV(mode == WResourceAcquireMode::PointerOnly || IsResourceTypeAcquireDuringUpdateContentAllowed(pCurrentlyUpdatingContent->GetDynamicRTTI(), WGetStaticRTTI<ResourceType>()),
      "Trying to acquire a resource of type '{0}' during '{1}::UpdateContent()'. This has to be enabled by calling "
      "WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<{1}, {0}>(); at engine startup, for example in "
      "WGameApplication::Init_SetupDefaultResources().",
      WGetStaticRTTI<ResourceType>()->GetTypeName(), pCurrentlyUpdatingContent->GetDynamicRTTI()->GetTypeName());
  }
#endif

  ResourceType* pResource = (ResourceType*)hResource.m_hTypeless.m_pResource;

  // W_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  W_ASSERT_DEBUG(pResource->GetDynamicRTTI()->template IsDerivedFrom<ResourceType>(),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    WGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == WResourceAcquireMode::AllowLoadingFallback && GetForceNoFallbackAcquisition() > 0)
  {
    mode = WResourceAcquireMode::BlockTillLoaded;
  }

  if (mode == WResourceAcquireMode::PointerOnly)
  {
    if (out_pAcquireResult)
      *out_pAcquireResult = WResourceAcquireResult::Final;

    // pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used
  // productively
  pResource->m_LastAcquire = GetLastFrameUpdate();

  if (pResource->GetLoadingState() != WResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != WResourceState::Loaded)
    {
      // if BlockTillLoaded is specified, it will prepended to the preload array, thus will be loaded immediately
      InternalPreloadResource(pResource, mode >= WResourceAcquireMode::BlockTillLoaded);

      if (mode == WResourceAcquireMode::AllowLoadingFallback &&
          (pResource->m_hLoadingFallback.IsValid() || hFallbackResource.IsValid() || GetResourceTypeLoadingFallback<ResourceType>().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_pAcquireResult)
          *out_pAcquireResult = WResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hLoadingFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hLoadingFallback, WResourceAcquireMode::BlockTillLoaded);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, WResourceAcquireMode::BlockTillLoaded);
        else
          return (ResourceType*)BeginAcquireResource(GetResourceTypeLoadingFallback<ResourceType>(), WResourceAcquireMode::BlockTillLoaded);
      }

      EnsureResourceLoadingState(pResource, WResourceState::Loaded);
    }
    else
    {
      // as long as there are more quality levels available, schedule the resource for more loading
      // accessing IsQueuedForLoading without a lock here is save because InternalPreloadResource() will lock and early out if necessary
      // and accidentally skipping InternalPreloadResource() is no problem
      if (IsQueuedForLoading(pResource) == false && pResource->GetNumQualityLevelsLoadable() > 0)
      {
        InternalPreloadResource(pResource, false);

        if (GetForceNoFallbackAcquisition() > 0)
        {
          EnsureResourceCondition(pResource, [=]() -> bool
            { return ((WInt32)pResource->GetLoadingState() >= (WInt32)WResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0) ||
                     (pResource->GetLoadingState() == WResourceState::LoadedResourceMissing); });
        }
      }
    }
  }

  if (pResource->GetLoadingState() == WResourceState::LoadedResourceMissing)
  {
    // When you get a crash with a stack overflow in this code path, then the resource to be used as the
    // 'missing resource' replacement might be missing itself.

    if (WResourceManager::GetResourceTypeMissingFallback<ResourceType>().IsValid())
    {
      if (out_pAcquireResult)
        *out_pAcquireResult = WResourceAcquireResult::MissingFallback;

      return (ResourceType*)BeginAcquireResource(
        WResourceManager::GetResourceTypeMissingFallback<ResourceType>(), WResourceAcquireMode::BlockTillLoaded);
    }

    if (mode != WResourceAcquireMode::AllowLoadingFallback_NeverFail && mode != WResourceAcquireMode::BlockTillLoaded_NeverFail)
    {
      W_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(),
        WGetStaticRTTI<ResourceType>()->GetTypeName());
    }

    if (out_pAcquireResult)
      *out_pAcquireResult = WResourceAcquireResult::None;

    return nullptr;
  }

  if (out_pAcquireResult)
    *out_pAcquireResult = WResourceAcquireResult::Final;

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
void WResourceManager::EndAcquireResource(ResourceType* pResource)
{
  W_IGNORE_UNUSED(pResource);
  // W_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (WInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

W_FORCE_INLINE void WResourceManager::EndAcquireResourcePointer(WResource* pResource)
{
  W_IGNORE_UNUSED(pResource);
  // W_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (WInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

template <typename ResourceType>
WLockedObject<WMutex, WDynamicArray<WResource*>> WResourceManager::GetAllResourcesOfType()
{
  const WRTTI* pBaseType = WGetStaticRTTI<ResourceType>();

  auto& container = GetLoadedResourceOfTypeTempContainer();

  // We use a static container here to ensure its life-time is extended beyond
  // calls to this function as the locked object does not own the passed-in object
  // and thus does not extend the data life-time. It is safe to do this, as the
  // locked object holding the container ensures the container will not be
  // accessed concurrently.
  WLockedObject<WMutex, WDynamicArray<WResource*>> loadedResourcesLock(s_ResourceMutex, &container);

  container.Clear();

  for (auto itType = GetLoadedResources().GetIterator(); itType.IsValid(); itType.Next())
  {
    const WRTTI* pDerivedType = itType.Key();

    if (pDerivedType->IsDerivedFrom(pBaseType))
    {
      const LoadedResources& lr = GetLoadedResources()[pDerivedType];

      container.Reserve(container.GetCount() + lr.m_Resources.GetCount());

      for (auto itResource : lr.m_Resources)
      {
        container.PushBack(itResource.Value());
      }
    }
  }

  return loadedResourcesLock;
}

template <typename ResourceType>
bool WResourceManager::ReloadResource(const WTypedResourceHandle<ResourceType>& hResource, bool bForce)
{
  ResourceType* pResource = BeginAcquireResource(hResource, WResourceAcquireMode::PointerOnly);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResource(pResource);

  return res;
}

W_FORCE_INLINE bool WResourceManager::ReloadResource(const WRTTI* pType, const WTypelessResourceHandle& hResource, bool bForce)
{
  WResource* pResource = BeginAcquireResourcePointer(pType, hResource);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResourcePointer(pResource);

  return res;
}

template <typename ResourceType>
WUInt32 WResourceManager::ReloadResourcesOfType(bool bForce)
{
  return ReloadResourcesOfType(WGetStaticRTTI<ResourceType>(), bForce);
}

template <typename ResourceType>
void WResourceManager::SetResourceTypeLoader(WResourceTypeLoader* pCreator)
{
  W_LOCK(s_ResourceMutex);

  GetResourceTypeLoaders()[WGetStaticRTTI<ResourceType>()] = pCreator;
}

template <typename ResourceType>
WTypedResourceHandle<ResourceType> WResourceManager::GetResourceHandleForExport(WStringView sResourceID)
{
  W_ASSERT_DEV(IsExportModeEnabled(), "Export mode needs to be enabled");

  return LoadResource<ResourceType>(sResourceID);
}

template <typename ResourceType>
void WResourceManager::SetIncrementalUnloadForResourceType(bool bActive)
{
  GetResourceTypeInfo(WGetStaticRTTI<ResourceType>()).m_bIncrementalUnload = bActive;
}

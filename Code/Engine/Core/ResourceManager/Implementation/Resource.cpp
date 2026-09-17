#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/StackTracer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WResource, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResource::DoUpdate WResource::UpdateGraphicsResource = WResource::DoUpdate::OnAnyThread;

W_CORE_DLL void IncreaseResourceRefCount(WResource* pResource, const void* pOwner)
{
#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
  {
    W_LOCK(pResource->m_HandleStackTraceMutex);

    auto& info = pResource->m_HandleStackTraces[pOwner];

    WArrayPtr<void*> ptr(info.m_Ptrs);

    info.m_uiNumPtrs = WStackTracer::GetStackTrace(ptr);
  }
#else
  W_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Increment();
}

W_CORE_DLL void DecreaseResourceRefCount(WResource* pResource, const void* pOwner)
{
#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
  {
    W_LOCK(pResource->m_HandleStackTraceMutex);

    if (!pResource->m_HandleStackTraces.Remove(pOwner, nullptr))
    {
      W_REPORT_FAILURE("No associated stack-trace!");
    }
  }
#else
  W_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Decrement();
}

#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
W_CORE_DLL void MigrateResourceRefCount(WResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  W_LOCK(pResource->m_HandleStackTraceMutex);

  // allocate / resize the hash-table first to ensure the iterator stays valid
  auto& newInfo = pResource->m_HandleStackTraces[pNewOwner];

  auto it = pResource->m_HandleStackTraces.Find(pOldOwner);
  if (!it.IsValid())
  {
    W_REPORT_FAILURE("No associated stack-trace!");
  }
  else
  {
    newInfo = it.Value();
    pResource->m_HandleStackTraces.Remove(it);
  }
}
#endif

WResource::~WResource()
{
  W_ASSERT_DEV(!WResourceManager::IsQueuedForLoading(this), "Cannot deallocate a resource while it is still qeued for loading");
}

WResource::WResource(DoUpdate ResourceUpdateThread, WUInt8 uiQualityLevelsLoadable)
{
  if (ResourceUpdateThread == DoUpdate::OnGraphicsResourceThreads)
  {
    ResourceUpdateThread = UpdateGraphicsResource;
  }

  m_Flags.AddOrRemove(WResourceFlags::UpdateOnMainThread, ResourceUpdateThread == DoUpdate::OnMainThread);

  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
}

#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
static void LogStackTrace(const char* szText)
{
  WLog::Info(szText);
};
#endif

void WResource::PrintHandleStackTraces()
{
#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)

  W_LOCK(m_HandleStackTraceMutex);

  W_LOG_BLOCK("Resource Handle Stack Traces");

  for (auto& it : m_HandleStackTraces)
  {
    W_LOG_BLOCK("Handle Trace");

    WStackTracer::ResolveStackTrace(WArrayPtr<void*>(it.Value().m_Ptrs, it.Value().m_uiNumPtrs), LogStackTrace);
  }

#else

  WLog::Warning("Compile with W_RESOURCEHANDLE_STACK_TRACES set to W_ON to enable support for resource handle stack traces.");

#endif
}

void WResource::SetResourceDescription(WStringView sDescription)
{
  m_sResourceDescription = sDescription;
}

void WResource::SetUniqueID(WStringView sUniqueID, bool bIsReloadable)
{
  m_sUniqueID = sUniqueID;
  m_uiUniqueIDHash = WHashingUtils::StringHash(sUniqueID);
  SetIsReloadable(bIsReloadable);

  WResourceEvent e;
  e.m_pResource = this;
  e.m_Type = WResourceEvent::Type::ResourceCreated;
  WResourceManager::BroadcastResourceEvent(e);
}

void WResource::CallUnloadData(Unload WhatToUnload)
{
  W_LOG_BLOCK("WResource::UnloadData", GetResourceID());

  WResourceEvent e;
  e.m_pResource = this;
  e.m_Type = WResourceEvent::Type::ResourceContentUnloading;
  WResourceManager::BroadcastResourceEvent(e);

  WResourceLoadDesc ld = UnloadData(WhatToUnload);

  W_ASSERT_DEV(ld.m_State != WResourceState::Invalid, "UnloadData() did not return a valid resource load state");
  W_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsDiscardable correctly");
  W_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
thread_local const WResource* g_pCurrentlyUpdatingContent = nullptr;

const WResource* WResource::GetCurrentlyUpdatingContent()
{
  return g_pCurrentlyUpdatingContent;
}
#endif

void WResource::CallUpdateContent(WStreamReader* Stream)
{
  W_PROFILE_SCOPE("CallUpdateContent");

  W_LOG_BLOCK("WResource::UpdateContent", GetResourceDescription());

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WResource* pPreviouslyUpdatingContent = g_pCurrentlyUpdatingContent;
  g_pCurrentlyUpdatingContent = this;
  WResourceLoadDesc ld = UpdateContent(Stream);
  g_pCurrentlyUpdatingContent = pPreviouslyUpdatingContent;
#else
  WResourceLoadDesc ld = UpdateContent(Stream);
#endif

  W_ASSERT_DEV(ld.m_State != WResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  W_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  W_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  if (ld.m_State == WResourceState::LoadedResourceMissing)
  {
    ReportResourceIsMissing();
  }

  IncResourceChangeCounter();

  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
  m_LoadingState = ld.m_State;

  WResourceEvent e;
  e.m_pResource = this;
  e.m_Type = WResourceEvent::Type::ResourceContentUpdated;
  WResourceManager::BroadcastResourceEvent(e);

  WLog::Debug("Updated {0} - '{1}'", GetDynamicRTTI()->GetTypeName(), WArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

float WResource::GetLoadingPriority(WTime now) const
{
  if (m_Priority == WResourcePriority::Critical)
    return 0.0f;

  // low priority values mean it gets loaded earlier
  float fPriority = static_cast<float>(m_Priority) * 10.0f;

  if (GetLoadingState() == WResourceState::Loaded)
  {
    // already loaded -> more penalty
    fPriority += 30.0f;

    // the more it could discard, the less important it is to load more of it
    fPriority += GetNumQualityLevelsDiscardable() * 10.0f;
  }
  else
  {
    const WBitflags<WResourceFlags> flags = GetBaseResourceFlags();

    if (flags.IsAnySet(WResourceFlags::ResourceHasFallback))
    {
      // if the resource has a very specific fallback, it is least important to be get loaded
      fPriority += 20.0f;
    }
    else if (flags.IsAnySet(WResourceFlags::ResourceHasTypeFallback))
    {
      // if it has at least a type fallback, it is less important to get loaded
      fPriority += 10.0f;
    }
  }

  // everything acquired in the last N seconds gets a higher priority
  // by getting the lowest penalty
  const float secondsSinceAcquire = (float)(now - GetLastAcquireTime()).GetSeconds();
  const float fTimePriority = WMath::Min(10.0f, secondsSinceAcquire);

  return fPriority + fTimePriority;
}

void WResource::SetPriority(WResourcePriority priority)
{
  if (m_Priority == priority)
    return;

  m_Priority = priority;

  WResourceEvent e;
  e.m_pResource = this;
  e.m_Type = WResourceEvent::Type::ResourcePriorityChanged;
  WResourceManager::BroadcastResourceEvent(e);
}

WResourceTypeLoader* WResource::GetDefaultResourceTypeLoader() const
{
  return WResourceManager::GetDefaultResourceLoader();
}

void WResource::ReportResourceIsMissing()
{
  WLog::SeriousWarning("Missing Resource of Type '{2}': '{0}' ('{1}')", WArgSensitive(GetResourceID(), "ResourceID"),
    WArgSensitive(m_sResourceDescription, "ResourceDesc"), GetDynamicRTTI()->GetTypeName());
}

void WResource::VerifyAfterCreateResource(const WResourceLoadDesc& ld)
{
  W_ASSERT_DEV(ld.m_State != WResourceState::Invalid, "CreateResource() did not return a valid resource load state");
  W_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsDiscardable correctly");
  W_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsLoadable correctly");

  IncResourceChangeCounter();

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;

  /* Update Memory Usage*/
  {
    WResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    UpdateMemoryUsage(MemUsage);

    W_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", GetResourceID());
    W_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", GetResourceID());

    m_MemoryUsage = MemUsage;
  }

  WResourceEvent e;
  e.m_pResource = this;
  e.m_Type = WResourceEvent::Type::ResourceContentUpdated;
  WResourceManager::BroadcastResourceEvent(e);

  WLog::Debug("Created {0} - '{1}' ", GetDynamicRTTI()->GetTypeName(), WArgSensitive(GetResourceIdOrDescription(), "ResourceDesc"));
}

W_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);

#include <RendererCore/RendererCorePCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

WCVarBool cvar_RenderingMultithreading("Rendering.Multithreading", true, WCVarFlags::Default, "Enables multi-threaded update and rendering");
WCVarBool cvar_RenderingCachingStaticObjects("Rendering.Caching.StaticObjects", true, WCVarFlags::Default, "Enables render data caching of static objects");

WEvent<WView*, WMutex> WRenderWorld::s_ViewCreatedEvent;
WEvent<WView*, WMutex> WRenderWorld::s_ViewDeletedEvent;

WEvent<void*> WRenderWorld::s_CameraConfigsModifiedEvent;
bool WRenderWorld::s_bModifyingCameraConfigs = false;
WMap<WString, WRenderWorld::CameraConfig> WRenderWorld::s_CameraConfigs;

WEvent<const WRenderWorldExtractionEvent&, WMutex> WRenderWorld::s_ExtractionEvent;
WEvent<const WRenderWorldRenderEvent&, WMutex> WRenderWorld::s_RenderEvent;
WUInt64 WRenderWorld::s_uiFrameCounter;

namespace
{
  static bool s_bInExtract;
  static WThreadID s_RenderingThreadID;

  static WMutex s_ExtractTasksMutex;
  static WDynamicArray<WTaskGroupID> s_ExtractTasks;

  static WMutex s_ViewsMutex;
  static WIdTable<WViewId, WView*> s_Views;

  static WDynamicArray<WViewHandle> s_MainViews;

  static WMutex s_ViewsToRenderMutex;
  static WDynamicArray<WView*> s_ViewsToRender;

  static WDynamicArray<WSharedPtr<WRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    W_DECLARE_POD_TYPE();

    WRenderPipeline* m_pPipeline;
    WViewHandle m_hView;
  };

  static WMutex s_PipelinesToRebuildMutex;
  static WDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;

  static WProxyAllocator* s_pCacheAllocator;

  static WMutex s_CachedRenderDataMutex;
  using CachedRenderDataPerComponent = WHybridArray<const WRenderData*, 4>;
  static WHashTable<WComponentHandle, CachedRenderDataPerComponent> s_CachedRenderData;
  static WDynamicArray<const WRenderData*> s_DeletedRenderData;

  enum
  {
    MaxNumNewCacheEntries = 32
  };

  static bool s_bWriteRenderPipelineDgml = false;
  static WConsoleFunction<void()> s_ConFunc_WriteRenderPipelineDgml("WriteRenderPipelineDgml", "()", []()
    { s_bWriteRenderPipelineDgml = true; });
} // namespace

namespace WInternal
{
  struct RenderDataCache
  {
    RenderDataCache(WAllocator* pAllocator)
      : m_PerObjectCaches(pAllocator)
    {
      for (WUInt32 i = 0; i < MaxNumNewCacheEntries; ++i)
      {
        m_NewEntriesPerComponent.PushBack(NewEntryPerComponent(pAllocator));
      }
    }

    struct PerObjectCache
    {
      PerObjectCache() = default;

      PerObjectCache(WAllocator* pAllocator)
        : m_Entries(pAllocator)
      {
      }

      WHybridArray<RenderDataCacheEntry, 4> m_Entries;
      WUInt16 m_uiVersion = 0;
      bool m_bHasDependencies = false;
    };

    struct PerObjectDependenciesCache
    {
      WSmallArray<WTextureDependency, 4> m_TextureDependencies;
      WSmallArray<WBufferDependency, 4> m_BufferDependencies;
      WUInt16 m_uiVersion = 0;
    };

    WDynamicArray<PerObjectCache> m_PerObjectCaches;
    WHashTable<WUInt32, PerObjectDependenciesCache> m_PerObjectDependenciesCache;

    struct NewEntryPerComponent
    {
      NewEntryPerComponent(WAllocator* pAllocator)
        : m_Cache(pAllocator)
      {
      }

      WGameObjectHandle m_hOwnerObject;
      WComponentHandle m_hOwnerComponent;
      PerObjectCache m_Cache;
      WSmallArray<WTextureDependency, 2> m_TextureDependencies;
      WSmallArray<WBufferDependency, 2> m_BufferDependencies;
    };

    WStaticArray<NewEntryPerComponent, MaxNumNewCacheEntries> m_NewEntriesPerComponent;
    WAtomicInteger32 m_NewEntriesCount;
  };

#if W_ENABLED(W_PLATFORM_64BIT)
  static_assert(sizeof(RenderDataCacheEntry) == 16);
#endif
} // namespace WInternal

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WRenderWorld::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WRenderWorld::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WViewHandle WRenderWorld::CreateView(WStringView sName, WView*& out_pView)
{
  WView* pView = W_DEFAULT_NEW(WView);

  {
    W_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(sName);

  pView->m_pRenderDataCache = W_NEW(s_pCacheAllocator, WInternal::RenderDataCache, s_pCacheAllocator);

  s_ViewCreatedEvent.Broadcast(pView);

  out_pView = pView;
  return pView->GetHandle();
}

void WRenderWorld::DeleteView(const WViewHandle& hView)
{
  WView* pView = nullptr;

  {
    W_LOCK(s_ViewsMutex);
    if (!s_Views.Remove(hView, &pView))
      return;
  }

  s_ViewDeletedEvent.Broadcast(pView);

  W_DELETE(s_pCacheAllocator, pView->m_pRenderDataCache);

  {
    W_LOCK(s_PipelinesToRebuildMutex);

    for (WUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAtAndCopy(i);
      }
    }
  }

  RemoveMainView(hView);

  W_DEFAULT_DELETE(pView);
}

bool WRenderWorld::TryGetView(const WViewHandle& hView, WView*& out_pView)
{
  W_LOCK(s_ViewsMutex);
  return s_Views.TryGetValue(hView, out_pView);
}

WView* WRenderWorld::GetViewByUsageHint(WCameraUsageHint::Enum usageHint, WCameraUsageHint::Enum alternativeUsageHint /*= WCameraUsageHint::None*/, const WWorld* pWorld /*= nullptr*/)
{
  W_LOCK(s_ViewsMutex);

  WView* pAlternativeView = nullptr;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    if (pWorld != nullptr && pView->GetWorld() != pWorld)
      continue;

    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
    else if (alternativeUsageHint != WCameraUsageHint::None && pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = pView;
    }
  }

  return pAlternativeView;
}

void WRenderWorld::AddMainView(const WViewHandle& hView)
{
  W_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(hView))
    s_MainViews.PushBack(hView);
}

void WRenderWorld::RemoveMainView(const WViewHandle& hView)
{
  WUInt32 uiIndex = s_MainViews.IndexOf(hView);
  if (uiIndex != WInvalidIndex)
  {
    W_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");
    s_MainViews.RemoveAtAndCopy(uiIndex);
  }
}

void WRenderWorld::ClearMainViews()
{
  W_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

WArrayPtr<WViewHandle> WRenderWorld::GetMainViews()
{
  return s_MainViews;
}

bool WRenderWorld::IsRenderingScheduled()
{
  return !s_MainViews.IsEmpty() || !s_FilteredRenderPipelines[GetDataIndexForRendering()].IsEmpty();
}

void WRenderWorld::CacheRenderData(const WView& view, const WGameObjectHandle& hOwnerObject, const WComponentHandle& hOwnerComponent, WUInt16 uiComponentVersion, WArrayPtr<WInternal::RenderDataCacheEntry> cacheEntries, WArrayPtr<const WTextureDependency> textureDependencies, WArrayPtr<const WBufferDependency> bufferDependencies)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    WUInt32 uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount;
    if (uiNewEntriesCount >= MaxNumNewCacheEntries)
    {
      return;
    }

    uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount.Increment();
    if (uiNewEntriesCount <= MaxNumNewCacheEntries)
    {
      auto& newEntry = view.m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntriesCount - 1];
      newEntry.m_hOwnerObject = hOwnerObject;
      newEntry.m_hOwnerComponent = hOwnerComponent;
      newEntry.m_Cache.m_Entries = cacheEntries;
      newEntry.m_Cache.m_uiVersion = uiComponentVersion;
      newEntry.m_TextureDependencies = textureDependencies;
      newEntry.m_BufferDependencies = bufferDependencies;
    }
  }
}

void WRenderWorld::DeleteAllCachedRenderData()
{
  W_PROFILE_SCOPE("DeleteAllCachedRenderData");

  W_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  {
    W_LOCK(s_ViewsMutex);

    for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
    {
      WView* pView = it.Value();
      pView->m_pRenderDataCache->m_PerObjectCaches.Clear();
      pView->m_pRenderDataCache->m_PerObjectDependenciesCache.Clear();
    }
  }

  {
    W_LOCK(s_CachedRenderDataMutex);

    for (auto it = s_CachedRenderData.GetIterator(); it.IsValid(); ++it)
    {
      auto& cachedRenderDataPerComponent = it.Value();

      for (auto pCachedRenderData : cachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      cachedRenderDataPerComponent.Clear();
    }
  }
}

void WRenderWorld::DeleteCachedRenderData(const WGameObjectHandle& hOwnerObject, const WComponentHandle& hOwnerComponent)
{
  W_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(hOwnerObject);

  W_LOCK(s_CachedRenderDataMutex);

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    s_CachedRenderData.Remove(hOwnerComponent);
  }
}

void WRenderWorld::ResetRenderDataCache(WView& ref_view)
{
  ref_view.m_pRenderDataCache->m_PerObjectCaches.Clear();
  ref_view.m_pRenderDataCache->m_PerObjectDependenciesCache.Clear();
  ref_view.m_pRenderDataCache->m_NewEntriesCount = 0;

  if (ref_view.GetWorld() != nullptr)
  {
    if (ref_view.GetWorld()->GetObjectDeletionEvent().HasEventHandler(&WRenderWorld::DeleteCachedRenderDataForObject) == false)
    {
      ref_view.GetWorld()->GetObjectDeletionEvent().AddEventHandler(&WRenderWorld::DeleteCachedRenderDataForObject);
    }
  }
}

void WRenderWorld::DeleteCachedRenderDataForObject(const WGameObject* pOwnerObject)
{
  W_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(pOwnerObject->GetHandle());

  W_LOCK(s_CachedRenderDataMutex);

  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    WComponentHandle hComponent = pComponent->GetHandle();

    CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
    if (s_CachedRenderData.TryGetValue(hComponent, pCachedRenderDataPerComponent))
    {
      for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      s_CachedRenderData.Remove(hComponent);
    }
  }
}

void WRenderWorld::DeleteCachedRenderDataForObjectRecursive(const WGameObject* pOwnerObject)
{
  DeleteCachedRenderDataForObject(pOwnerObject);

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataForObjectRecursive(it);
  }
}

WArrayPtr<const WInternal::RenderDataCacheEntry> WRenderWorld::GetCachedRenderData(const WView& view, const WGameObjectHandle& hOwner, WUInt16 uiComponentVersion, WArrayPtr<const WTextureDependency>& out_textureDependencies, WArrayPtr<const WBufferDependency>& out_bufferDependencies)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    const auto& perObjectCaches = view.m_pRenderDataCache->m_PerObjectCaches;
    WUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < perObjectCaches.GetCount())
    {
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion == uiComponentVersion)
      {
        if (perObjectCache.m_bHasDependencies)
        {
          const auto& perObjectDependenciesCaches = view.m_pRenderDataCache->m_PerObjectDependenciesCache;
          WUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;

          auto it = perObjectDependenciesCaches.Find(uiCacheIndex);
          if (it.IsValid() && it.Value().m_uiVersion == uiComponentVersion)
          {
            out_textureDependencies = it.Value().m_TextureDependencies;
            out_bufferDependencies = it.Value().m_BufferDependencies;
          }
        }

        return perObjectCache.m_Entries;
      }
    }
  }

  return WArrayPtr<const WInternal::RenderDataCacheEntry>();
}

void WRenderWorld::AddViewToRender(const WViewHandle& hView)
{
  WView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  {
    W_LOCK(s_ViewsToRenderMutex);
    W_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    // make sure the view is put at the end of the array, if it is already there, reorder it
    // this ensures that the views that have been referenced by the last other view, get rendered first
    WUInt32 uiIndex = s_ViewsToRender.IndexOf(pView);
    if (uiIndex != WInvalidIndex)
    {
      s_ViewsToRender.RemoveAtAndCopy(uiIndex);
      s_ViewsToRender.PushBack(pView);
      return;
    }

    s_ViewsToRender.PushBack(pView);
  }

  if (cvar_RenderingMultithreading)
  {
    WTaskGroupID extractTaskID = WTaskSystem::StartSingleTask(pView->GetExtractTask(), WTaskPriority::EarlyThisFrame);

    {
      W_LOCK(s_ExtractTasksMutex);
      s_ExtractTasks.PushBack(extractTaskID);
    }
  }
  else
  {
    pView->ExtractData();
  }
}

void WRenderWorld::AddViewDependency(const WView& consumerView, WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEV(s_bInExtract, "AddViewDependency must be called during extraction");

  if (consumerView.m_pRenderPipeline)
  {
    consumerView.m_pRenderPipeline->AddViewDependency(hTexture, requiredState, stage);
  }
}

void WRenderWorld::AddViewDependency(const WView& consumerView, WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEV(s_bInExtract, "AddViewDependency must be called during extraction");

  if (consumerView.m_pRenderPipeline)
  {
    consumerView.m_pRenderPipeline->AddViewDependency(hBuffer, requiredState, stage);
  }
}

void WRenderWorld::ExtractMainViews()
{
  W_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  // must happen before the BeginExtraction broadcast, since listeners may call AddViewToRender,
  // which appends the tasks that we have to wait for below
  if (cvar_RenderingMultithreading)
  {
    W_LOCK(s_ExtractTasksMutex);
    s_ExtractTasks.Clear();
  }

  WRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = WRenderWorldExtractionEvent::Type::BeginExtraction;
  extractionEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_ExtractionEvent.Broadcast(extractionEvent);

  if (cvar_RenderingMultithreading)
  {
    WTaskGroupID extractTaskID = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);
    s_ExtractTasks.PushBack(extractTaskID);

    {
      W_LOCK(s_ViewsMutex);

      for (WUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        WView* pView = nullptr;
        if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
        {
          s_ViewsToRender.PushBack(pView);
          WTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
        }
      }
    }

    WTaskSystem::StartTaskGroup(extractTaskID);

    {
      W_PROFILE_SCOPE("Wait for Extraction");

      while (true)
      {
        WTaskGroupID taskID;

        {
          W_LOCK(s_ExtractTasksMutex);
          if (s_ExtractTasks.IsEmpty())
            break;

          taskID = s_ExtractTasks.PeekBack();
          s_ExtractTasks.PopBack();
        }

        WTaskSystem::WaitForGroup(taskID);
      }
    }
  }
  else
  {
    for (WUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      WView* pView = nullptr;
      if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
      {
        s_ViewsToRender.PushBack(pView);
        pView->ExtractData();
      }
    }
  }

  // filter out duplicates and reverse order so that dependent views are rendered first
  {
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForExtraction()];
    filteredRenderPipelines.Clear();

    for (WUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      auto& pRenderPipeline = s_ViewsToRender[i]->m_pRenderPipeline;
      if (!filteredRenderPipelines.Contains(pRenderPipeline))
      {
        filteredRenderPipelines.PushBack(pRenderPipeline);
      }
    }

    s_ViewsToRender.Clear();
  }

  extractionEvent.m_Type = WRenderWorldExtractionEvent::Type::EndExtraction;
  s_ExtractionEvent.Broadcast(extractionEvent);

  s_bInExtract = false;
}

void WRenderWorld::Render(WRenderContext* pRenderContext)
{
  const WUInt64 uiRenderFrame = WRenderWorld::GetUseMultithreadedRendering() ? WRenderWorld::GetFrameCounter() - 1 : WRenderWorld::GetFrameCounter();
  WStringBuilder sb;
  sb.SetFormat("RENDER FRAME {}", uiRenderFrame);
  W_PROFILE_SCOPE(sb.GetData());

  WRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = WRenderWorldRenderEvent::Type::BeginRender;
  renderEvent.m_uiFrameCounter = s_uiFrameCounter;
  {
    W_PROFILE_SCOPE("BeginRender");
    s_RenderEvent.Broadcast(renderEvent);
  }

  if (!cvar_RenderingMultithreading)
  {
    RebuildPipelines();
  }

  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  if (s_bWriteRenderPipelineDgml)
  {
    // Executed via WriteRenderPipelineDgml console command.
    s_bWriteRenderPipelineDgml = false;
    const WDateTime dt = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());
    for (WUInt32 i = 0; i < filteredRenderPipelines.GetCount(); ++i)
    {
      auto& pRenderPipeline = filteredRenderPipelines[i];
      WStringBuilder sPath(":appdata/Profiling/", WApplication::GetApplicationInstance()->GetApplicationName());
      sPath.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}_Pipeline{}_{}.dgml", dt.GetYear(), WArgU(dt.GetMonth(), 2, true), WArgU(dt.GetDay(), 2, true), WArgU(dt.GetHour(), 2, true), WArgU(dt.GetMinute(), 2, true), WArgU(dt.GetSecond(), 2, true), i, pRenderPipeline->GetViewName().GetData());

      WDGMLGraph graph(WDGMLGraph::Direction::TopToBottom);
      pRenderPipeline->CreateDgmlGraph(graph);
      if (WDGMLGraphWriter::WriteGraphToFile(sPath, graph).Failed())
      {
        WLog::Error("Failed to write render pipeline dgml: {}", sPath);
      }
    }
  }

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted soon.
    if (pRenderPipeline->GetRefCount() > 1 && pRenderPipeline->ShouldRender())
    {
      pRenderPipeline->EnqueueRenderGraph(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }
  WRenderGraphManager::ExecuteRenderGraphs(WGALDevice::GetDefaultDevice());

  filteredRenderPipelines.Clear();
  /// NOTE: (Only Applies When Tracy is Enabled.)Tracy Seems to declare Timers in the same scope, so dual profile macros can throw: '__tracy_scoped_zone' : redefinition; multitple initalization, so we must scope the two events.
  {
    renderEvent.m_Type = WRenderWorldRenderEvent::Type::EndRender;
    W_PROFILE_SCOPE("EndRender");
    s_RenderEvent.Broadcast(renderEvent);
  }
}

void WRenderWorld::BeginFrame()
{
  W_PROFILE_SCOPE("BeginFrame");

  s_RenderingThreadID = WThreadUtils::GetCurrentThreadID();
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();


  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    pView->EnsureUpToDate();
  }


  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];
  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    WGALSwapChainHandle hSwapChain = pRenderPipeline->GetRenderData().GetViewData().m_hSwapChain;
    if (!hSwapChain.IsInvalidated())
    {
      pDevice->EnqueueFrameSwapChain(hSwapChain);
    }
  }

  const WUInt64 uiRenderFrame = WRenderWorld::GetUseMultithreadedRendering() ? WRenderWorld::GetFrameCounter() - 1 : WRenderWorld::GetFrameCounter();
  // This will acquire swap-chain textures and may have changed size
  {
    WLogBlock b("Device::BeginFrame");
    pDevice->BeginFrame(uiRenderFrame);
  }
  RebuildPipelines();
}

void WRenderWorld::EndFrame()
{
  W_PROFILE_SCOPE("WRenderWorld::EndFrame");
  WGALDevice::GetDefaultDevice()->EndFrame();

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  ClearRenderDataCache();
  UpdateRenderDataCache();

  s_RenderingThreadID = (WThreadID)0;
}

bool WRenderWorld::GetUseMultithreadedRendering()
{
  return cvar_RenderingMultithreading;
}


bool WRenderWorld::IsRenderingThread()
{
  return s_RenderingThreadID == WThreadUtils::GetCurrentThreadID();
}

void WRenderWorld::DeleteCachedRenderDataInternal(const WGameObjectHandle& hOwnerObject)
{
  WUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;
  WWorld* pWorld = WWorld::GetWorld(hOwnerObject);

  W_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    if (pView->GetWorld() != nullptr && pView->GetWorld() == pWorld)
    {
      auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

      if (uiCacheIndex < perObjectCaches.GetCount())
      {
        perObjectCaches[uiCacheIndex].m_Entries.Clear();
        perObjectCaches[uiCacheIndex].m_uiVersion = 0;
      }

      pView->m_pRenderDataCache->m_PerObjectDependenciesCache.Remove(uiCacheIndex);
    }
  }
}

void WRenderWorld::ClearRenderDataCache()
{
  W_PROFILE_SCOPE("Clear Render Data Cache");

  for (auto pRenderData : s_DeletedRenderData)
  {
    WRenderData* ptr = const_cast<WRenderData*>(pRenderData);
    W_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void WRenderWorld::UpdateRenderDataCache()
{
  W_PROFILE_SCOPE("Update Render Data Cache");

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    WUInt32 uiNumNewEntries = WMath::Min<WInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

    for (WUInt32 uiNewEntryIndex = 0; uiNewEntryIndex < uiNumNewEntries; ++uiNewEntryIndex)
    {
      auto& newEntries = pView->m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntryIndex];
      W_ASSERT_DEV(!newEntries.m_hOwnerObject.IsInvalidated(), "Implementation error");

      // find or create cached render data
      auto& cachedRenderDataPerComponent = s_CachedRenderData[newEntries.m_hOwnerComponent];

      const WUInt32 uiNumCachedRenderData = cachedRenderDataPerComponent.GetCount();
      if (uiNumCachedRenderData == 0) // Nothing cached yet
      {
        cachedRenderDataPerComponent = CachedRenderDataPerComponent(s_pCacheAllocator);
      }

      WUInt32 uiCachedRenderDataIndex = 0;
      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (newEntry.m_pRenderData != nullptr)
        {
          if (uiCachedRenderDataIndex >= cachedRenderDataPerComponent.GetCount())
          {
            const WRTTI* pRtti = newEntry.m_pRenderData->GetDynamicRTTI();
            newEntry.m_pRenderData = pRtti->GetAllocator()->Clone<WRenderData>(newEntry.m_pRenderData, s_pCacheAllocator);

            cachedRenderDataPerComponent.PushBack(newEntry.m_pRenderData);
          }
          else
          {
            // replace with cached render data
            newEntry.m_pRenderData = cachedRenderDataPerComponent[uiCachedRenderDataIndex];
          }

          ++uiCachedRenderDataIndex;
        }
      }

      // add entry for this view
      const WUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      perObjectCaches.EnsureCount(uiCacheIndex + 1);
      const bool bHasDependencies = !newEntries.m_TextureDependencies.IsEmpty() || !newEntries.m_BufferDependencies.IsEmpty();
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      perObjectCache.m_bHasDependencies = bHasDependencies;
      if (perObjectCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
      {
        perObjectCache.m_Entries.Clear();
        perObjectCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
      }

      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (!perObjectCache.m_Entries.Contains(newEntry))
        {
          perObjectCache.m_Entries.PushBack(newEntry);
        }
      }

      // Dependencies are sparse, so they are stored in a separate map keyed by the object's cache index.
      if (bHasDependencies)
      {
        auto& perObjectDependenciesCache = pView->m_pRenderDataCache->m_PerObjectDependenciesCache[uiCacheIndex];
        if (perObjectDependenciesCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
        {
          perObjectDependenciesCache.m_TextureDependencies.Clear();
          perObjectDependenciesCache.m_BufferDependencies.Clear();
          perObjectDependenciesCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
        }

        for (const WTextureDependency& dep : newEntries.m_TextureDependencies)
        {
          perObjectDependenciesCache.m_TextureDependencies.PushBack(dep);
        }

        for (const WBufferDependency& dep : newEntries.m_BufferDependencies)
        {
          perObjectDependenciesCache.m_BufferDependencies.PushBack(dep);
        }
      }

      // keep entries sorted, otherwise the logic WExtractor::ExtractRenderData doesn't work
      perObjectCache.m_Entries.Sort();
    }
  }
}

// static
void WRenderWorld::AddRenderPipelineToRebuild(WRenderPipeline* pRenderPipeline, const WViewHandle& hView)
{
  W_ASSERT_DEV(pRenderPipeline != nullptr, "Pipeline must not be null");

  W_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_hView == hView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_hView = hView;
}

// static
void WRenderWorld::RebuildPipelines()
{
  W_PROFILE_SCOPE("RebuildPipelines");

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    WView* pView = nullptr;
    if (s_Views.TryGetValue(pipelineToRebuild.m_hView, pView))
    {
      if (pipelineToRebuild.m_pPipeline->Rebuild(*pView) == WRenderPipeline::PipelineState::RebuildError)
      {
        WLog::Error("Failed to rebuild pipeline '{}' for view '{}'", pipelineToRebuild.m_pPipeline->m_sName, pView->GetName());
      }
    }
  }

  s_PipelinesToRebuild.Clear();
}

void WRenderWorld::OnEngineStartup()
{
  s_pCacheAllocator = W_DEFAULT_NEW(WProxyAllocator, "Cached Render Data", WFoundation::GetDefaultAllocator());

  s_CachedRenderData = WHashTable<WComponentHandle, CachedRenderDataPerComponent>(s_pCacheAllocator);
}

void WRenderWorld::OnEngineShutdown()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  for (auto it : s_CachedRenderData)
  {
    auto& cachedRenderDataPerComponent = it.Value();
    if (cachedRenderDataPerComponent.IsEmpty() == false)
    {
      W_REPORT_FAILURE("Leaked cached render data of type '{}'", cachedRenderDataPerComponent[0]->GetDynamicRTTI()->GetTypeName());
    }
  }
#endif

  ClearRenderDataCache();

  W_DEFAULT_DELETE(s_pCacheAllocator);

  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  ClearMainViews();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    WView* pView = it.Value();
    W_DEFAULT_DELETE(pView);
  }

  s_Views.Clear();
  s_CameraConfigs.Clear();
}

void WRenderWorld::BeginModifyCameraConfigs()
{
  W_ASSERT_DEBUG(!s_bModifyingCameraConfigs, "Recursive call not allowed.");
  s_bModifyingCameraConfigs = true;
}

void WRenderWorld::EndModifyCameraConfigs()
{
  W_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call WRenderWorld::BeginModifyCameraConfigs first");
  s_bModifyingCameraConfigs = false;
  s_CameraConfigsModifiedEvent.Broadcast(nullptr);
}

void WRenderWorld::ClearCameraConfigs()
{
  W_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call WRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs.Clear();
}

void WRenderWorld::SetCameraConfig(const char* szName, const CameraConfig& config)
{
  W_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call WRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs[szName] = config;
}

const WRenderWorld::CameraConfig* WRenderWorld::FindCameraConfig(const char* szName)
{
  auto it = s_CameraConfigs.Find(szName);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

W_STATICLINK_FILE(RendererCore, RendererCore_RenderWorld_Implementation_RenderWorld);

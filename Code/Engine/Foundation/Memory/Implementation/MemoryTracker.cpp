#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && TRACY_ENABLE && TRACY_ENABLE_MEMORY_TRACKING
#  include <tracy/tracy/Tracy.hpp>

#  define W_TRACY_CALLSTACK_DEPTH 16
#  define W_TRACY_ALLOC_CS(ptr, size, name) TracyAllocNS(ptr, size, W_TRACY_CALLSTACK_DEPTH, name)
#  define W_TRACY_FREE_CS(ptr, name) TracyFreeNS(ptr, W_TRACY_CALLSTACK_DEPTH, name)
#  define W_TRACY_ALLOC(ptr, size, name) TracyAllocN(ptr, size, name)
#  define W_TRACY_FREE(ptr, name) TracyFreeN(ptr, name)
#else
#  define W_TRACY_ALLOC_CS(ptr, size, name)
#  define W_TRACY_FREE_CS(ptr, name)
#  define W_TRACY_ALLOC(ptr, size, name)
#  define W_TRACY_FREE(ptr, name)
#endif

namespace
{
  // no tracking for the tracker data itself
  using TrackerDataAllocator = WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::Nothing>;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    W_ALWAYS_INLINE static WAllocator* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    W_ALWAYS_INLINE AllocatorData() = default;

    WHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    WAllocatorTrackingMode m_TrackingMode;

    WAllocatorId m_ParentId;

    WAllocator::Stats m_Stats;

    WHashTable<const void*, WMemoryTracker::AllocationInfo, WHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    W_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    W_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    WMutex m_Mutex;

    using AllocatorTable = WIdTable<WAllocatorId, AllocatorData, TrackerDataAllocatorWrapper>;
    AllocatorTable m_AllocatorData;
  };

  static TrackerData* s_pTrackerData;
  static bool s_bIsInitialized = false;
  static bool s_bIsInitializing = false;

  static void Initialize()
  {
    if (s_bIsInitialized)
      return;

    W_ASSERT_DEV(!s_bIsInitializing, "MemoryTracker initialization entered recursively");
    s_bIsInitializing = true;

    if (s_pTrackerDataAllocator == nullptr)
    {
      alignas(alignof(TrackerDataAllocator)) static WUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      W_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      alignas(alignof(TrackerData)) static WUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      W_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized = true;
    s_bIsInitializing = false;
  }

  static void DumpLeak(const WMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char szBuffer[512];
    WUInt64 uiSize = info.m_uiSize;
    WStringUtils::snprintf(szBuffer, W_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);

    WLog::Print(szBuffer);

    if (info.GetStackTrace().GetPtr() != nullptr)
    {
      WStackTracer::ResolveStackTrace(info.GetStackTrace(), &WLog::Print);
    }

    WLog::Print("--------------------------------------------------------------------\n\n");
  }
} // namespace

// Iterator
#define CAST_ITER(ptr) static_cast<TrackerData::AllocatorTable::Iterator*>(ptr)

WAllocatorId WMemoryTracker::Iterator::Id() const
{
  return CAST_ITER(m_pData)->Id();
}

WStringView WMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName;
}

WAllocatorId WMemoryTracker::Iterator::ParentId() const
{
  return CAST_ITER(m_pData)->Value().m_ParentId;
}

const WAllocator::Stats& WMemoryTracker::Iterator::Stats() const
{
  return CAST_ITER(m_pData)->Value().m_Stats;
}

void WMemoryTracker::Iterator::Next()
{
  CAST_ITER(m_pData)->Next();
}

bool WMemoryTracker::Iterator::IsValid() const
{
  return CAST_ITER(m_pData)->IsValid();
}

WMemoryTracker::Iterator::~Iterator()
{
  auto it = CAST_ITER(m_pData);
  W_DELETE(s_pTrackerDataAllocator, it);
  m_pData = nullptr;
}


// static
WAllocatorId WMemoryTracker::RegisterAllocator(WStringView sName, WAllocatorTrackingMode mode, WAllocatorId parentId)
{
  Initialize();

  W_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = sName;
  data.m_TrackingMode = mode;
  data.m_ParentId = parentId;

  return s_pTrackerData->m_AllocatorData.Insert(data);
}

// static
void WMemoryTracker::DeregisterAllocator(WAllocatorId allocatorId)
{
  W_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  WUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0 && data.m_TrackingMode > WAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }

    W_REPORT_FAILURE("Allocator '{0}' leaked {1} allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

// static
void WMemoryTracker::AddAllocation(WAllocatorId allocatorId, WAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, WTime allocationTime)
{
  W_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  WArrayPtr<void*> stackTrace;
  if (mode >= WAllocatorTrackingMode::AllocationStatsAndStacktraces)
  {
    void* pBuffer[64];
    WArrayPtr<void*> tempTrace(pBuffer);
    const WUInt32 uiNumTraces = WStackTracer::GetStackTrace(tempTrace);

    stackTrace = W_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces);
    WMemoryUtils::Copy(stackTrace.GetPtr(), pBuffer, uiNumTraces);
  }

  {
    W_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
    data.m_Stats.m_uiNumAllocations++;
    data.m_Stats.m_uiAllocationSize += uiSize;
    data.m_Stats.m_uiPerFrameAllocationSize += uiSize;
    data.m_Stats.m_PerFrameAllocationTime += allocationTime;

    auto pInfo = &data.m_Allocations[pPtr];
    pInfo->m_uiSize = uiSize;
    pInfo->m_uiAlignment = (WUInt16)uiAlign;
    pInfo->SetStackTrace(stackTrace);

    if (mode >= WAllocatorTrackingMode::AllocationStatsAndStacktraces)
    {
      W_TRACY_ALLOC_CS(pPtr, uiSize, data.m_sName.GetData());
    }
    else
    {
      W_TRACY_ALLOC(pPtr, uiSize, data.m_sName.GetData());
    }
  }
}

// static
void WMemoryTracker::RemoveAllocation(WAllocatorId allocatorId, const void* pPtr)
{
  WArrayPtr<void*> stackTrace;

  {
    W_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

    AllocationInfo info;
    if (data.m_Allocations.Remove(pPtr, &info))
    {
      data.m_Stats.m_uiNumDeallocations++;
      data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

      stackTrace = info.GetStackTrace();

      if (data.m_TrackingMode >= WAllocatorTrackingMode::AllocationStatsAndStacktraces)
      {
        W_TRACY_FREE_CS(pPtr, data.m_sName.GetData());
      }
      else
      {
        W_TRACY_FREE(pPtr, data.m_sName.GetData());
      }
    }
    else
    {
      W_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", WArgP(pPtr));
    }
  }

  W_DELETE_ARRAY(s_pTrackerDataAllocator, stackTrace);
}

// static
void WMemoryTracker::RemoveAllAllocations(WAllocatorId allocatorId)
{
  W_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

    if (data.m_TrackingMode >= WAllocatorTrackingMode::AllocationStatsAndStacktraces)
    {
      for (const auto& alloc : data.m_Allocations)
      {
        W_IGNORE_UNUSED(alloc);
        W_TRACY_FREE_CS(alloc.Key(), data.m_sName.GetData());
      }
    }
    else
    {
      for (const auto& alloc : data.m_Allocations)
      {
        W_IGNORE_UNUSED(alloc);
        W_TRACY_FREE(alloc.Key(), data.m_sName.GetData());
      }
    }

    W_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void WMemoryTracker::SetAllocatorStats(WAllocatorId allocatorId, const WAllocator::Stats& stats)
{
  W_LOCK(*s_pTrackerData);

  s_pTrackerData->m_AllocatorData[allocatorId].m_Stats = stats;
}

// static
void WMemoryTracker::ResetPerFrameAllocatorStats()
{
  W_LOCK(*s_pTrackerData);

  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    AllocatorData& data = it.Value();
    data.m_Stats.m_uiPerFrameAllocationSize = 0;
    data.m_Stats.m_PerFrameAllocationTime = WTime::MakeZero();
  }
}

// static
WStringView WMemoryTracker::GetAllocatorName(WAllocatorId allocatorId)
{
  W_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName;
}

// static
const WAllocator::Stats& WMemoryTracker::GetAllocatorStats(WAllocatorId allocatorId)
{
  W_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

// static
WAllocatorId WMemoryTracker::GetAllocatorParentId(WAllocatorId allocatorId)
{
  W_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_ParentId;
}

// static
const WMemoryTracker::AllocationInfo& WMemoryTracker::GetAllocationInfo(WAllocatorId allocatorId, const void* pPtr)
{
  W_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(pPtr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  W_REPORT_FAILURE("Could not find info for allocation {0}", WArgP(pPtr));
  return invalidInfo;
}

struct LeakInfo
{
  W_DECLARE_POD_TYPE();

  WAllocatorId m_AllocatorId;
  size_t m_uiSize = 0;
  bool m_bIsRootLeak = true;
};

// static
WUInt32 WMemoryTracker::PrintMemoryLeaks(PrintFunc printfunc)
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return 0;

  W_LOCK(*s_pTrackerData);

  WHashTable<const void*, LeakInfo, WHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize = it2.Value().m_uiSize;

      if (data.m_TrackingMode == WAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        leak.m_bIsRootLeak = false;
      }

      leakTable.Insert(it2.Key(), leak);
    }
  }

  // find dependencies
  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    const void* curPtr = ptr;
    const void* endPtr = WMemoryUtils::AddByteOffset(ptr, leak.m_uiSize);

    while (curPtr < endPtr)
    {
      const void* testPtr = *reinterpret_cast<const void* const*>(curPtr);

      LeakInfo* dependentLeak = nullptr;
      if (leakTable.TryGetValue(testPtr, dependentLeak))
      {
        dependentLeak->m_bIsRootLeak = false;
      }

      curPtr = WMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  WUInt32 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.m_bIsRootLeak)
    {
      const AllocatorData& data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];

      if (data.m_TrackingMode != WAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        if (uiNumLeaks == 0)
        {
          printfunc("\n\n--------------------------------------------------------------------\n"
                    "Memory Leak Report:"
                    "\n--------------------------------------------------------------------\n\n");
        }

        WMemoryTracker::AllocationInfo info;
        data.m_Allocations.TryGetValue(ptr, info);

        DumpLeak(info, data.m_sName.GetData());

        ++uiNumLeaks;
      }
    }
  }

  if (uiNumLeaks > 0)
  {
    char tmp[1024];
    WStringUtils::snprintf(tmp, 1024, "\n--------------------------------------------------------------------\n"
                                       "Found %u root memory leak(s)."
                                       "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);

    printfunc(tmp);
  }

  return uiNumLeaks;
}

// static
void WMemoryTracker::DumpMemoryLeaks()
{
  const WUInt32 uiNumLeaks = PrintMemoryLeaks(WLog::Print);

  if (uiNumLeaks > 0)
  {
    W_REPORT_FAILURE("Found {0} root memory leak(s). See console output for details.", uiNumLeaks);
  }
}

// static
WMemoryTracker::Iterator WMemoryTracker::GetIterator()
{
  auto pInnerIt = W_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}

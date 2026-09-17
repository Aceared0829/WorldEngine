#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Bitflags.h>

enum class WAllocatorTrackingMode : WUInt32
{
  Nothing,                       ///< The allocator doesn't track anything. Use this for best performance.
  Basics,                        ///< The allocator will be known to the system, so it can show up in debugging tools, but barely anything more.
  AllocationStats,               ///< The allocator keeps track of how many allocations and deallocations it did and how large its memory usage is.
  AllocationStatsIgnoreLeaks,    ///< Same as AllocationStats, but any remaining allocations at shutdown are not reported as leaks.
  AllocationStatsAndStacktraces, ///< The allocator will record stack traces for each allocation, which can be used to find memory leaks.

  Default = W_ALLOC_TRACKING_DEFAULT,
};

/// Global memory tracking system for debugging, profiling, and leak detection.
///
/// This singleton provides comprehensive memory allocation tracking across all allocators
/// in the system. It supports different tracking modes ranging from basic statistics to
/// full stack trace recording for every allocation.
class W_FOUNDATION_DLL WMemoryTracker
{
public:
  struct AllocationInfo
  {
    W_DECLARE_POD_TYPE();

    W_FORCE_INLINE AllocationInfo() = default;

    void** m_pStackTrace = nullptr;
    size_t m_uiSize = 0;
    WUInt16 m_uiAlignment = 0;
    WUInt16 m_uiStackTraceLength = 0;

    W_ALWAYS_INLINE const WArrayPtr<void*> GetStackTrace() const { return WArrayPtr<void*>(m_pStackTrace, (WUInt32)m_uiStackTraceLength); }

    W_ALWAYS_INLINE WArrayPtr<void*> GetStackTrace() { return WArrayPtr<void*>(m_pStackTrace, (WUInt32)m_uiStackTraceLength); }

    W_FORCE_INLINE void SetStackTrace(WArrayPtr<void*> stackTrace)
    {
      m_pStackTrace = stackTrace.GetPtr();
      W_ASSERT_DEV(stackTrace.GetCount() < 0xFFFF, "stack trace too long");
      m_uiStackTraceLength = (WUInt16)stackTrace.GetCount();
    }
  };

  class W_FOUNDATION_DLL Iterator
  {
  public:
    ~Iterator();

    WAllocatorId Id() const;
    WStringView Name() const;
    WAllocatorId ParentId() const;
    const WAllocator::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    W_ALWAYS_INLINE void operator++() { Next(); }

  private:
    friend class WMemoryTracker;

    W_ALWAYS_INLINE Iterator(void* pData)
      : m_pData(pData)
    {
    }

    void* m_pData;
  };

  static WAllocatorId RegisterAllocator(WStringView sName, WAllocatorTrackingMode mode, WAllocatorId parentId);
  static void DeregisterAllocator(WAllocatorId allocatorId);

  static void AddAllocation(WAllocatorId allocatorId, WAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, WTime allocationTime);
  static void RemoveAllocation(WAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(WAllocatorId allocatorId);
  static void SetAllocatorStats(WAllocatorId allocatorId, const WAllocator::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static WStringView GetAllocatorName(WAllocatorId allocatorId);
  static const WAllocator::Stats& GetAllocatorStats(WAllocatorId allocatorId);
  static WAllocatorId GetAllocatorParentId(WAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(WAllocatorId allocatorId, const void* pPtr);

  static Iterator GetIterator();

  /// Callback for printing strings.
  using PrintFunc = void (*)(const char* szLine);

  /// Reports back information about all currently known root memory leaks.
  ///
  /// Returns the number of found memory leaks.
  static WUInt32 PrintMemoryLeaks(PrintFunc printfunc);

  /// Prints the known memory leaks to WLog and triggers an assert if there are any.
  ///
  /// This is useful to call at the end of an application, to get a debug breakpoint in case of memory leaks.
  static void DumpMemoryLeaks();
};

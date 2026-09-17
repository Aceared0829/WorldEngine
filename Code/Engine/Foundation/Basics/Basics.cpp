#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/CommonAllocators.h>

#if W_ENABLED(W_ALLOC_GUARD_ALLOCATIONS)
using DefaultHeapType = WGuardingAllocator;
using DefaultAlignedHeapType = WGuardingAllocator;
using DefaultStaticsHeapType = WAllocatorWithPolicy<WAllocPolicyGuarding, WAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#else
using DefaultHeapType = WHeapAllocator;
using DefaultAlignedHeapType = WAlignedHeapAllocator;
using DefaultStaticsHeapType = WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#endif

enum
{
  HEAP_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultHeapType),
  ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultAlignedHeapType),
};

alignas(W_ALIGNMENT_MINIMUM) static WUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
alignas(W_ALIGNMENT_MINIMUM) static WUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

alignas(W_ALIGNMENT_MINIMUM) static WUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool WFoundation::s_bIsInitialized = false;
WAllocator* WFoundation::s_pDefaultAllocator = nullptr;
WAllocator* WFoundation::s_pAlignedAllocator = nullptr;

void WFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WMemoryUtils::ReserveLower4GBAddressSpace();
#endif

  if (s_pDefaultAllocator == nullptr)
  {
    s_pDefaultAllocator = new (s_DefaultAllocatorBuffer) DefaultHeapType("DefaultHeap");
  }

  if (s_pAlignedAllocator == nullptr)
  {
    s_pAlignedAllocator = new (s_AlignedAllocatorBuffer) DefaultAlignedHeapType("AlignedHeap");
  }

  s_bIsInitialized = true;
}

#if defined(W_CUSTOM_STATIC_ALLOCATOR_FUNC)
extern WAllocator* W_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

WAllocator* WFoundation::GetStaticsAllocator()
{
  static WAllocator* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(W_CUSTOM_STATIC_ALLOCATOR_FUNC)

#  if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

#    if W_ENABLED(W_PLATFORM_WINDOWS)
    using GetStaticAllocatorFunc = WAllocator* (*)();

    HMODULE hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, W_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
#    else
#      error "Customizing static allocator not implemented"
#    endif

#  else
    return W_CUSTOM_STATIC_ALLOCATOR_FUNC();
#  endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) DefaultStaticsHeapType("Statics");
  }

  return pStaticAllocator;
}

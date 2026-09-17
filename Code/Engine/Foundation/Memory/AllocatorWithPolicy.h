#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

W_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, WHasReallocate);

#include <Foundation/Memory/Implementation/AllocatorMixin_inl.h>

/// Policy-based allocator that combines allocation strategies with tracking modes.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, WAllocatorTrackingMode TrackingMode = WAllocatorTrackingMode::Default>
class WAllocatorWithPolicy : public WInternal::WAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
                                WHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  WAllocatorWithPolicy(WStringView sName, WAllocator* pParent = nullptr)
    : WInternal::WAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
        WHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};

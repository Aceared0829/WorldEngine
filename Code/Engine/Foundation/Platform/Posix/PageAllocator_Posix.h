#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>

// static
void* WPageAllocator::AllocatePage(size_t uiSize)
{
  WTime fAllocationTime = WTime::Now();

  void* ptr = nullptr;
  size_t uiAlign = WSystemInformation::Get().GetMemoryPageSize();
  const int res = posix_memalign(&ptr, uiAlign, uiSize);
  W_ASSERT_DEBUG(res == 0, "Failed to align pointer");
  W_IGNORE_UNUSED(res);

  W_CHECK_ALIGNMENT(ptr, uiAlign);

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::AddAllocation(WPageAllocator::GetId(), WAllocatorTrackingMode::Default, ptr, uiSize, uiAlign, WTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void WPageAllocator::DeallocatePage(void* ptr)
{
  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllocation(WPageAllocator::GetId(), ptr);
  }

  free(ptr);
}

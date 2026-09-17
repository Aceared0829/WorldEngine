#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Memory/MemoryTracker.h>
#  include <Foundation/Memory/PageAllocator.h>
#  include <Foundation/System/SystemInformation.h>
#  include <Foundation/Time/Time.h>

// static
void* WPageAllocator::AllocatePage(size_t uiSize)
{
  WTime fAllocationTime = WTime::Now();

  void* ptr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  W_ASSERT_DEV(ptr != nullptr, "Could not allocate memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));

  size_t uiAlign = WSystemInformation::Get().GetMemoryPageSize();
  W_CHECK_ALIGNMENT(ptr, uiAlign);

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::AddAllocation(WPageAllocator::GetId(), WAllocatorTrackingMode::Default, ptr, uiSize, uiAlign, WTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void WPageAllocator::DeallocatePage(void* pPtr)
{
  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllocation(WPageAllocator::GetId(), pPtr);
  }

  W_VERIFY(::VirtualFree(pPtr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));
}

#endif

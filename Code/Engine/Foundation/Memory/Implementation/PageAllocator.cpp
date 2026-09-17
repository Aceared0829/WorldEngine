#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>

WAllocatorId WPageAllocator::GetId()
{
  static WAllocatorId id;

  if (id.IsInvalidated())
  {
    id = WMemoryTracker::RegisterAllocator("Page", WAllocatorTrackingMode::Default, WAllocatorId());
  }

  return id;
}

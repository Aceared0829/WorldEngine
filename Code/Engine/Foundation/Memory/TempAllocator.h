#pragma once

#include <Foundation/Memory/Allocator.h>

/// Stack-based allocator for temporary allocations.
///
/// This allocator is designed for short-lived allocations that ideally follow a LIFO pattern but can also handle out-of-order deallocations.
class W_FOUNDATION_DLL WTempAllocator
{
public:
  W_ALWAYS_INLINE static WAllocator* Get() { return s_pAllocator; }

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TempAllocator);

  static void Startup();
  static void Shutdown();

  static WAllocator* s_pAllocator;
};

/// Wrapper for the allocator that is used for temporary allocations.
struct WTempAllocatorWrapper
{
  W_ALWAYS_INLINE static WAllocator* GetAllocator() { return WTempAllocator::Get(); }
};

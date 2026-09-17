#pragma once

#include <Foundation/Memory/Allocator.h>

/// Allocator wrapper that should never be used - causes assertion failures.
///
/// This wrapper is used as a template parameter to indicate that no allocator
/// should be used. Any attempt to call GetAllocator() will trigger an assertion.
/// Useful for container types that should never allocate.
struct WNullAllocatorWrapper
{
  W_FORCE_INLINE static WAllocator* GetAllocator()
  {
    W_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

/// Wrapper for the engine's default general-purpose allocator.
struct WDefaultAllocatorWrapper
{
  W_ALWAYS_INLINE static WAllocator* GetAllocator() { return WFoundation::GetDefaultAllocator(); }
};

/// Wrapper for the allocator used for static/global objects.
struct WStaticsAllocatorWrapper
{
  W_ALWAYS_INLINE static WAllocator* GetAllocator() { return WFoundation::GetStaticsAllocator(); }
};

/// Wrapper for the allocator that provides memory with specific alignment guarantees.
struct WAlignedAllocatorWrapper
{
  W_ALWAYS_INLINE static WAllocator* GetAllocator() { return WFoundation::GetAlignedAllocator(); }
};

/// Helper function to facilitate setting the allocator on member containers of a class
/// Allocators can be either template arguments or a ctor parameter. Using the ctor parameter requires the class ctor to reference each member container in the initialization list. This can be very tedious. On the other hand, the template variant only support template parameter so you can't simply pass in a member allocator.
/// This class solves this problem provided the following rules are followed:
/// 1. The `WAllocator` must be the declared at the earliest in the class, before any container.
/// 2. The `WLocalAllocatorWrapper` should be declared right afterwards.
/// 3. Any container needs to be declared below these two and must include the `WLocalAllocatorWrapper` as a template argument to the allocator.
/// 4. In the ctor initializer list, init the WAllocator first, then the WLocalAllocatorWrapper. With this approach all containers can be omitted.
/// \code{.cpp}
///   class MyClass
///   {
///     WAllocator m_SpecialAlloc;
///     WLocalAllocatorWrapper m_Wrapper;
///
///     WDynamicArray<int, WLocalAllocatorWrapper> m_Data;
///
///     MyClass()
///       : m_SpecialAlloc("MySpecialAlloc")
///       , m_Wrapper(&m_SpecialAlloc)
///     {
///     }
///   }
/// \endcode
struct W_FOUNDATION_DLL WLocalAllocatorWrapper
{
  WLocalAllocatorWrapper(WAllocator* pAllocator);

  void Reset();

  static WAllocator* GetAllocator();
};

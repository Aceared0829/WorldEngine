#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyStack.h>
#include <Foundation/Memory/TempAllocator.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TempAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    WTempAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WTempAllocator::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WAllocator* WTempAllocator::s_pAllocator;

// static
void WTempAllocator::Startup()
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnFree = true;
#else
  static constexpr bool OverwriteMemoryOnFree = false;
#endif
  using StackAllocatorType = WAllocatorWithPolicy<WAllocPolicyStack<OverwriteMemoryOnFree>, WAllocatorTrackingMode::Basics>;

  s_pAllocator = W_DEFAULT_NEW(StackAllocatorType, "TempAllocator", WFoundation::GetAlignedAllocator());
}

// static
void WTempAllocator::Shutdown()
{
  W_DEFAULT_DELETE(s_pAllocator);
}


W_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_TempAllocator);

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/Thread.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch WRunnable instances
void* WThreadClassEntryPoint(void* pThreadParameter)
{
  W_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  WThread* pThread = reinterpret_cast<WThread*>(pThreadParameter);

  RunThread(pThread);

  return nullptr;
}

/// \endcond

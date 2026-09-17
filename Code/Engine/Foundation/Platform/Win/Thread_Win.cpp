#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Threading/Thread.h>

// Thread entry point used to launch WRunnable instances
DWORD __stdcall WThreadClassEntryPoint(LPVOID pThreadParameter)
{
  W_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  WThread* pThread = reinterpret_cast<WThread*>(pThreadParameter);

  return RunThread(pThread);
}

#endif

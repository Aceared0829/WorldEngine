#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Mutex.h>

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

template <WUInt32 a, WUInt32 b>
struct SameSize
{
  static_assert(a == b, "Critical section has incorrect size");
};

template <WUInt32 a, WUInt32 b>
struct SameAlignment
{
  static_assert(a == b, "Critical section has incorrect alignment");
};


WMutex::WMutex()
{
  SameSize<sizeof(WMutexHandle), sizeof(CRITICAL_SECTION)> check1;
  (void)check1;
  SameAlignment<alignof(WMutexHandle), alignof(CRITICAL_SECTION)> check2;
  (void)check2;
  InitializeCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

WMutex::~WMutex()
{
  DeleteCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}
#endif

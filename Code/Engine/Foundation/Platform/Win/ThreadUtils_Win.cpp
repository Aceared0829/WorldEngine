#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Threading/ThreadUtils.h>
#  include <Foundation/Time/Time.h>

static DWORD g_uiMainThreadID = 0xFFFFFFFF;

void WThreadUtils::Initialize()
{
  g_uiMainThreadID = GetCurrentThreadId();
}

void WThreadUtils::YieldTimeSlice()
{
  ::Sleep(0);
}

void WThreadUtils::YieldHardwareThread()
{
  YieldProcessor();
}

void WThreadUtils::Sleep(const WTime& duration)
{
  ::Sleep((DWORD)duration.GetMilliseconds());
}

WThreadID WThreadUtils::GetCurrentThreadID()
{
  return ::GetCurrentThreadId();
}

bool WThreadUtils::IsMainThread()
{
  return GetCurrentThreadID() == g_uiMainThreadID;
}

#endif

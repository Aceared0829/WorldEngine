#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

// Posix implementation of thread helper functions

#include <pthread.h>

static pthread_t g_MainThread = (pthread_t)0;

void WThreadUtils::Initialize()
{
  g_MainThread = pthread_self();
}

void WThreadUtils::YieldTimeSlice()
{
  sched_yield();
}

void WThreadUtils::YieldHardwareThread()
{
  // No equivalent to mm_pause on linux
}

void WThreadUtils::Sleep(const WTime& duration)
{
  timespec SleepTime;
  SleepTime.tv_sec = duration.GetSeconds();
  SleepTime.tv_nsec = ((WInt64)duration.GetMilliseconds() * 1000000LL) % 1000000000LL;
  nanosleep(&SleepTime, nullptr);
}

// WThreadHandle WThreadUtils::GetCurrentThreadHandle()
//{
//  return pthread_self();
//}

WThreadID WThreadUtils::GetCurrentThreadID()
{
  return pthread_self();
}

bool WThreadUtils::IsMainThread()
{
  return pthread_self() == g_MainThread;
}

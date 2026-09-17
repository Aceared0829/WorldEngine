#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Time/Time.h>

#include <time.h>

void WTime::Initialize()
{
}

WTime WTime::Now()
{
  struct timespec sp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &sp);

  return WTime::MakeFromSeconds((double)sp.tv_sec + (double)(sp.tv_nsec / 1000000000.0));
}

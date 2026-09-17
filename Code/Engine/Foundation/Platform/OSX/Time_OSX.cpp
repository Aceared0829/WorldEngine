#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_OSX)

#  include <CoreServices/CoreServices.h>
#  include <mach/mach.h>
#  include <mach/mach_time.h>

static double g_TimeFactor = 0;

void WTime::Initialize()
{
  mach_timebase_info_data_t TimebaseInfo;
  mach_timebase_info(&TimebaseInfo);
  g_TimeFactor = (double)TimebaseInfo.numer / (double)TimebaseInfo.denom / (double)1000000000LL;
}

WTime WTime::Now()
{
  // mach_absolute_time() returns nanoseconds after factoring in the mach_timebase_info_data_t
  return WTime::MakeFromSeconds((double)mach_absolute_time() * g_TimeFactor);
}

#endif

#include <Foundation/Platform/PlatformDesc.h>

WPlatformDesc g_PlatformDescOSX("OSX", "Desktop");

#if W_ENABLED(W_PLATFORM_OSX)

const WPlatformDesc* WPlatformDesc::s_pThisPlatform = &g_PlatformDescOSX;

#endif

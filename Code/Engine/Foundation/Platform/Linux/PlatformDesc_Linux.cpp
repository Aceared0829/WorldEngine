#include <Foundation/Platform/PlatformDesc.h>

WPlatformDesc g_PlatformDescLinux("Linux", "Desktop");

#if W_ENABLED(W_PLATFORM_LINUX)

const WPlatformDesc* WPlatformDesc::s_pThisPlatform = &g_PlatformDescLinux;

#endif

#include <Foundation/Platform/PlatformDesc.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WPlatformDesc);

WPlatformDesc g_PlatformDescWin("Windows", "Desktop");

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

const WPlatformDesc* WPlatformDesc::s_pThisPlatform = &g_PlatformDescWin;

#endif

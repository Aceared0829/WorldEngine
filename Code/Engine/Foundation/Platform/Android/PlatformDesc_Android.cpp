#include <Foundation/Platform/PlatformDesc.h>

WPlatformDesc g_PlatformDescAndroid("Android", "Mobile");

#if W_ENABLED(W_PLATFORM_ANDROID)

const WPlatformDesc* WPlatformDesc::s_pThisPlatform = &g_PlatformDescAndroid;

#endif

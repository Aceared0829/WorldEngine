#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics/Platform/PlatformFeatures.h>
#include <Foundation/System/Screen.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

WResult WScreen::EnumerateScreens(WDynamicArray<WScreenInfo>& out_Screens)
{
  if (ANativeWindow* pWindow = WAndroidUtils::GetAndroidApp()->window)
  {
    WScreenInfo& currentScreen = out_Screens.ExpandAndGetRef();
    currentScreen.m_sDisplayName = "Current Display";
    currentScreen.m_iOffsetX = 0;
    currentScreen.m_iOffsetY = 0;
    currentScreen.m_iResolutionX = ANativeWindow_getWidth(pWindow);
    currentScreen.m_iResolutionY = ANativeWindow_getHeight(pWindow);
    currentScreen.m_bIsPrimary = true;
    return W_SUCCESS;
  }
  return W_FAILURE;
}
#endif

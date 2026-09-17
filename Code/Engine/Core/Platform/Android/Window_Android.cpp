#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/System/Screen.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <android_native_app_glue.h>

struct ANativeWindow;

namespace
{
  ANativeWindow* s_androidWindow = nullptr;
  WEventSubscriptionID s_androidCommandID = 0;
} // namespace

WWindowAndroid::~WWindowAndroid()
{
  DestroyWindow();
}

WResult WWindowAndroid::InitializeWindow()
{
  W_LOG_BLOCK("WWindowAndroid::Initialize", m_CreationDescription.m_Title.GetData());
  if (m_bInitialized)
  {
    DestroyWindow();
  }

  if (m_CreationDescription.m_WindowMode == WWindowMode::WindowResizable)
  {
    s_androidCommandID = WAndroidUtils::s_AppCommandEvent.AddEventHandler([this](WInt32 iCmd)
      {
      if (iCmd == APP_CMD_WINDOW_RESIZED)
      {
        WTempHybridArray<WScreenInfo, 2> screens;
        if (WScreen::EnumerateScreens(screens).Succeeded())
        {
          m_CreationDescription.m_Resolution.width = screens[0].m_iResolutionX;
          m_CreationDescription.m_Resolution.height = screens[0].m_iResolutionY;
          this->OnResize(WSizeU32(screens[0].m_iResolutionX, screens[0].m_iResolutionY));
        }
      } });
  }

  // Checking and adjustments to creation desc.
  if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
    WLog::Warning("Failed to adjust window size and position settings.");

  W_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  W_ASSERT_RELEASE(s_androidWindow == nullptr, "Window already exists. Only one Android window is supported at any time!");

  s_androidWindow = WAndroidUtils::GetAndroidApp()->window;
  m_hWindowHandle = s_androidWindow;
  m_pInputDevice = W_DEFAULT_NEW(WInputDevice_Android);
  m_bInitialized = true;

  return W_SUCCESS;
}

void WWindowAndroid::DestroyWindow()
{
  if (!m_bInitialized)
    return;

  W_LOG_BLOCK("WWindowAndroid::Destroy");

  s_androidWindow = nullptr;

  if (s_androidCommandID != 0)
  {
    WAndroidUtils::s_AppCommandEvent.RemoveEventHandler(s_androidCommandID);
  }

  WLog::Success("Window destroyed.");
}

WResult WWindowAndroid::Resize(const WSizeU32& newWindowSize)
{
  // No need to resize on Android, swapchain can take any size at any time.
  m_CreationDescription.m_Resolution.width = newWindowSize.width;
  m_CreationDescription.m_Resolution.height = newWindowSize.height;
  return W_SUCCESS;
}

void WWindowAndroid::ProcessWindowMessages()
{
  W_ASSERT_RELEASE(s_androidWindow != nullptr, "No window data available.");
}

WWindowHandle WWindowAndroid::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
